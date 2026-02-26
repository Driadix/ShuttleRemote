#include "CommLink.h"
#include <Arduino.h>
#include "Logger.h"
#include "DebugUtils.h"

CommLink::CommLink(ITransport* transport, TelemetryModel* model)
    : _transport(transport), _model(model),
      _jobHead(0), _jobTail(0), _txState(TxState::IDLE), _nextSeqNum(0)
{
    memset(_jobQueue, 0, sizeof(_jobQueue));
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
            if (header->targetID != _model->getTargetShuttleID() &&
                header->targetID != SP::TARGET_ID_BROADCAST) {
                return;
            }

            LOG_D("COMM", "RX: [%s] Seq: %d", DebugUtils::getMsgIdName(header->msgID), header->seq);

            _model->updateLastRxTime();
            EventBus::publish(SystemEvent::COMM_RX_ACTIVITY);

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
    if (_txState == TxState::WAITING_ACK && _jobHead != _jobTail) {
        if (_jobQueue[_jobHead].seqNum == seq) {
            uint8_t msgID = _jobQueue[_jobHead].packetData[6];
            if (isUserCommand(msgID)) {
                EventBus::publish(SystemEvent::CMD_ACKED);
            }
            _jobHead = (_jobHead + 1) % MAX_JOBS;
            _txState = TxState::IDLE;
        }
    }
}

bool CommLink::hasPendingHeartbeat() const {
    uint8_t idx = _jobHead;
    while (idx != _jobTail) {
        if (_jobQueue[idx].packetData[6] == SP::MSG_REQ_HEARTBEAT) return true;
        idx = (idx + 1) % MAX_JOBS;
    }
    return false;
}

bool CommLink::enqueuePacket(uint8_t msgID, const void* payload, uint8_t payloadLen, uint8_t maxRetries, uint16_t ackTimeoutMs) {
    uint8_t nextTail = (_jobTail + 1) % MAX_JOBS;
    if (nextTail == _jobHead) return false;

    uint16_t totalLen = sizeof(SP::FrameHeader) + payloadLen + 2; 
    if (totalLen > 128) return false; 

    TxJob* job = &_jobQueue[_jobTail];

    SP::FrameHeader header;
    header.sync1 = SP::PROTOCOL_SYNC_1;
    header.sync2 = SP::PROTOCOL_SYNC_2;
    header.length = payloadLen;
    header.targetID = _model->getTargetShuttleID();
    header.seq = _nextSeqNum++;
    header.msgID = msgID;

    memcpy(job->packetData, &header, sizeof(header));
    if (payloadLen > 0) {
        memcpy(job->packetData + sizeof(header), payload, payloadLen);
    }

    uint16_t crc = SP::ProtocolUtils::calcCRC16(job->packetData, sizeof(header) + payloadLen);
    job->packetData[sizeof(header) + payloadLen]     = (uint8_t)(crc & 0xFF);
    job->packetData[sizeof(header) + payloadLen + 1] = (uint8_t)((crc >> 8) & 0xFF);

    job->length = totalLen;
    job->seqNum = header.seq;
    job->retryCount = 0;
    job->maxRetries = maxRetries;
    job->ackTimeoutMs = ackTimeoutMs;
    job->cancelled = false;

    _jobTail = nextTail;
    return true;
}

void CommLink::processTxQueue() {
    if (!_transport) return;

    while (_jobHead != _jobTail && _jobQueue[_jobHead].cancelled) {
         _jobHead = (_jobHead + 1) % MAX_JOBS;
         _txState = TxState::IDLE;
    }

    if (_txState == TxState::IDLE) {
        if (_jobHead != _jobTail) {
            TxJob* job = &_jobQueue[_jobHead];
            if (_transport->availableForWrite() >= job->length) {
                _transport->write(job->packetData, job->length);
                job->lastTxTime = millis();
                _txState = TxState::WAITING_ACK;
            }
        }
    } else if (_txState == TxState::WAITING_ACK) {
        if (_jobHead != _jobTail) {
            TxJob* job = &_jobQueue[_jobHead];
            if (millis() - job->lastTxTime > job->ackTimeoutMs) {
                if (job->retryCount < job->maxRetries) {
                    if (_transport->availableForWrite() >= job->length) {
                        job->retryCount++;
                        _transport->write(job->packetData, job->length);
                        job->lastTxTime = millis();
                    }
                } else {
                    uint8_t msgID = job->packetData[6];
                    if (isUserCommand(msgID)) {
                        EventBus::publish(SystemEvent::CMD_FAILED);
                    }
                    _txState = TxState::TIMEOUT_ERROR;
                }
            }
        } else {
            _txState = TxState::IDLE;
        }
    } else if (_txState == TxState::TIMEOUT_ERROR) {
        if (_jobHead != _jobTail) {
            _jobHead = (_jobHead + 1) % MAX_JOBS;
        }
        _txState = TxState::IDLE;
    }
}

bool CommLink::sendCommand(SP::CommandPacket packet, uint8_t maxRetries, uint16_t ackTimeoutMs) {
    return enqueuePacket(SP::MSG_COMMAND, &packet, sizeof(packet), maxRetries, ackTimeoutMs);
}

bool CommLink::sendRequest(uint8_t msgID, uint8_t maxRetries, uint16_t ackTimeoutMs) {
    return enqueuePacket(msgID, nullptr, 0, maxRetries, ackTimeoutMs); 
}

bool CommLink::sendConfigSet(uint8_t paramID, int32_t value) {
    SP::ConfigPacket cfg;
    cfg.paramID = paramID;
    cfg.value = value;
    return enqueuePacket(SP::MSG_CONFIG_SET, &cfg, sizeof(cfg), 2, 500); 
}

bool CommLink::sendConfigGet(uint8_t paramID) {
    SP::ConfigPacket cfg;
    cfg.paramID = paramID;
    cfg.value = 0;
    return enqueuePacket(SP::MSG_CONFIG_GET, &cfg, sizeof(cfg), 2, 500);
}

bool CommLink::sendFullConfigSync(const SP::FullConfigPacket& config) {
    return enqueuePacket(SP::MSG_CONFIG_SYNC_PUSH, &config, sizeof(SP::FullConfigPacket), 3, 1000);
}

bool CommLink::isQueueFull() const {
    uint8_t nextTail = (_jobTail + 1) % MAX_JOBS;
    return nextTail == _jobHead;
}

bool CommLink::isWaitingForAck() const {
    return _txState == TxState::WAITING_ACK;
}

void CommLink::clearPendingCommands() {
    uint8_t idx = _jobHead;
    while (idx != _jobTail) {
        TxJob* job = &_jobQueue[idx];
        uint8_t msgID = job->packetData[6];

        if (isUserCommand(msgID)) {
            job->cancelled = true;
            if (_txState == TxState::WAITING_ACK && idx == _jobHead) {
                _txState = TxState::IDLE;
            } 
        }
        idx = (idx + 1) % MAX_JOBS;
    }
}

bool CommLink::isUserCommand(uint8_t msgID) const {
    return (msgID == SP::MSG_COMMAND || msgID == SP::MSG_CONFIG_SET || msgID == SP::MSG_CONFIG_GET || msgID == SP::MSG_CONFIG_SYNC_PUSH);
}