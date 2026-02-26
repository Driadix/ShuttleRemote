#include "CommLink.h"
#include <Arduino.h>
#include "Logger.h"
#include "DebugUtils.h"

CommLink::CommLink(ITransport* transport, TelemetryModel* model)
    : _transport(transport), _model(model),
      _trackedLength(0), _lastCmdTxTime(0), _lastSentCmdType(0),
      _txState(TxState::IDLE), _nextSeqNum(0)
{
    memset(_trackedBuffer, 0, sizeof(_trackedBuffer));
}

void CommLink::tick() {
    handleRx();
    processTxQueue();
}

void CommLink::handleRx() {
    if (!_transport) return;

    while (_transport->available()) {
        uint8_t byte = _transport->read();
        SP::FrameHeader* header = _parser.feed(byte);

        if (_parser.crcError) {
            LOG_W("COMM", "RX dropped: CRC mismatch");
        }

        if (header) {
            if (header->targetID != SP::TARGET_ID_NONE &&
                header->targetID != SP::TARGET_ID_BROADCAST) {
                return; // Drop if it's not meant for the Display
            }

            // ANY valid packet proves connection is alive.
            _model->updateLastRxTime();
            EventBus::publish(SystemEvent::COMM_RX_ACTIVITY);

            LOG_D("COMM", "RX: [%s] Seq: %d", DebugUtils::getMsgIdName(header->msgID), header->seq);

            uint8_t* payload = (uint8_t*)header + sizeof(SP::FrameHeader);

            switch (header->msgID) {
                case SP::MSG_ACK:
                    processIncomingAck(header->seq, (SP::AckPacket*)payload);
                    break;
                case SP::MSG_HEARTBEAT:
                    if (header->length == sizeof(SP::TelemetryPacket)) {
                        _model->updateTelemetry(*(SP::TelemetryPacket*)payload);
                    }
                    break;
                case SP::MSG_SENSORS:
                    if (header->length == sizeof(SP::SensorPacket)) {
                        _model->updateSensors(*(SP::SensorPacket*)payload);
                    }
                    break;
                case SP::MSG_STATS:
                    if (header->length == sizeof(SP::StatsPacket)) {
                        _model->updateStats(*(SP::StatsPacket*)payload);
                    }
                    break;
                case SP::MSG_CONFIG_REP:
                    if (header->length == sizeof(SP::ConfigPacket)) {
                        SP::ConfigPacket* pkt = (SP::ConfigPacket*)payload;
                        _model->updateConfig(pkt->paramID, pkt->value);
                    }
                    break;
                case SP::MSG_CONFIG_SYNC_REP:
                    if (header->length == sizeof(SP::FullConfigPacket)) {
                        _model->updateFullConfig(*(SP::FullConfigPacket*)payload);
                    }
                    break;
            }
        }
    }
}

void CommLink::processIncomingAck(uint8_t seq, SP::AckPacket* ack) {
    if (_txState == TxState::WAITING_ACK && seq == _trackedSeq) {
        uint8_t msgID = _trackedBuffer[6];
        if (isUserCommand(msgID)) {
            EventBus::publish(SystemEvent::CMD_ACKED);
        }
        _txState = TxState::IDLE; // Cleared!
    }
}

// Formats a raw packet and sends it to the transport layer instantly
void CommLink::transmitRawPacket(uint8_t msgID, const void* payload, uint8_t payloadLen, uint8_t* outBuffer, uint16_t& outLen) {
    SP::FrameHeader header;
    header.sync1 = SP::PROTOCOL_SYNC_1;
    header.sync2 = SP::PROTOCOL_SYNC_2;
    header.length = payloadLen;
    header.targetID = _model->getTargetShuttleID();
    header.seq = _nextSeqNum++;
    header.msgID = msgID;

    outLen = sizeof(header) + payloadLen + 2;
    memcpy(outBuffer, &header, sizeof(header));
    if (payloadLen > 0) {
        memcpy(outBuffer + sizeof(header), payload, payloadLen);
    }

    uint16_t crc = SP::ProtocolUtils::calcCRC16(outBuffer, outLen - 2);
    outBuffer[outLen - 2] = (uint8_t)(crc & 0xFF);
    outBuffer[outLen - 1] = (uint8_t)((crc >> 8) & 0xFF);

    LOG_D("COMM", "TX: [%s] Len: %d", DebugUtils::getMsgIdName(msgID), payloadLen);

    if (_transport && _transport->availableForWrite() >= outLen) {
        _transport->write(outBuffer, outLen);
    }
}

// DUAL-SLOT ARCHITECTURE: Automatically routes based on maxRetries requirement
bool CommLink::trackAndTransmit(uint8_t msgID, const void* payload, uint8_t payloadLen, uint8_t maxRetries, uint16_t timeout) {
    if (maxRetries == 0) {
        // SLOT 1: ACTION CHANNEL (Volatile)
        // Fire and forget. Transmits instantly, bypasses the Reliable Tracker entirely.
        uint8_t tempBuf[64];
        uint16_t tempLen;
        transmitRawPacket(msgID, payload, payloadLen, tempBuf, tempLen);
        return true;
    }

    // SLOT 2: RELIABLE CHANNEL (Tracked)
    transmitRawPacket(msgID, payload, payloadLen, _trackedBuffer, _trackedLength);
    
    _trackedSeq = _nextSeqNum - 1; 
    _trackedMaxRetries = maxRetries;
    _trackedRetries = 0;
    _trackedTimeout = timeout;
    _trackedTxTime = millis();
    _txState = TxState::WAITING_ACK;
    
    return true;
}

// Fire and forget polling (No ACKs, no retries, no queueing)
bool CommLink::sendRequest(uint8_t msgID) {
    uint8_t tempBuf[32];
    uint16_t tempLen;
    transmitRawPacket(msgID, nullptr, 0, tempBuf, tempLen);
    return true;
}

// Action Dispatcher (Dual-Slot + Throttled)
bool CommLink::sendCommand(SP::CommandPacket packet, uint8_t maxRetries, uint16_t ackTimeoutMs) {
    // GLOBAL THROTTLE: Protect 9600-baud bandwidth against generic button mashing
    if (packet.cmdType != SP::CMD_STOP && (millis() - _lastCmdTxTime < 300)) {
        LOG_D("COMM", "Command dropped (Global Throttle Active)");
        return true; // Return true so UI acts like it sent
    }

    // RELIABLE CHANNEL PROTECTION: Do not overwrite critical configs if we requested a reliable action
    if (_txState == TxState::WAITING_ACK && maxRetries > 0) {
        uint8_t trackedMsgID = _trackedBuffer[6];
        if (trackedMsgID == SP::MSG_CONFIG_SET || trackedMsgID == SP::MSG_CONFIG_SYNC_PUSH) {
            LOG_W("COMM", "Reliable action dropped; critical config currently in transit.");
            return false; 
        }
    }

    _lastCmdTxTime = millis();
    _lastSentCmdType = packet.cmdType;

    // Dispatcher automatically routes to Slot 1 (Volatile) or Slot 2 (Reliable)
    return trackAndTransmit(SP::MSG_COMMAND, &packet, sizeof(packet), maxRetries, ackTimeoutMs);
}

// Settings Dispatcher
bool CommLink::sendConfigSet(uint8_t paramID, int32_t value) {
    SP::ConfigPacket cfg;
    cfg.paramID = paramID;
    cfg.value = value;
    return trackAndTransmit(SP::MSG_CONFIG_SET, &cfg, sizeof(cfg), 2, 1000); 
}

bool CommLink::sendConfigGet(uint8_t paramID) {
    SP::ConfigPacket cfg;
    cfg.paramID = paramID;
    cfg.value = 0;
    return trackAndTransmit(SP::MSG_CONFIG_GET, &cfg, sizeof(cfg), 2, 1000);
}

bool CommLink::sendFullConfigSync(const SP::FullConfigPacket& config) {
    return trackAndTransmit(SP::MSG_CONFIG_SYNC_PUSH, &config, sizeof(SP::FullConfigPacket), 3, 1000);
}

// Handles timeout logic for the single active tracked packet in the Reliable Slot
void CommLink::processTxQueue() {
    if (!_transport) return;

    if (_txState == TxState::WAITING_ACK) {
        if (millis() - _trackedTxTime > _trackedTimeout) {
            if (_trackedRetries < _trackedMaxRetries) {
                if (_transport->availableForWrite() >= _trackedLength) {
                    _trackedRetries++;
                    _transport->write(_trackedBuffer, _trackedLength);
                    _trackedTxTime = millis();
                }
            } else {
                uint8_t msgID = _trackedBuffer[6];
                if (isUserCommand(msgID)) {
                    EventBus::publish(SystemEvent::CMD_FAILED);
                }
                _txState = TxState::IDLE; // Stop waiting
            }
        }
    }
}

bool CommLink::isWaitingForAck() const {
    return _txState == TxState::WAITING_ACK;
}

void CommLink::clearPendingCommands() {
    if (_txState == TxState::WAITING_ACK) {
        uint8_t msgID = _trackedBuffer[6];
        if (isUserCommand(msgID)) {
            _txState = TxState::IDLE;
        }
    }
}

bool CommLink::isUserCommand(uint8_t msgID) const {
    return (msgID == SP::MSG_COMMAND || msgID == SP::MSG_CONFIG_SET || msgID == SP::MSG_CONFIG_GET || msgID == SP::MSG_CONFIG_SYNC_PUSH);
}