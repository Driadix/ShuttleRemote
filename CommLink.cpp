#include "CommLink.h"
#include <Arduino.h>

CommLink::CommLink(ITransport* transport, TelemetryModel* model)
    : _transport(transport), _model(model), _txWriteIndex(0),
      _jobHead(0), _jobTail(0), _txState(TxState::IDLE), _nextSeqNum(0)
{
    memset(_txRingBuffer, 0, sizeof(_txRingBuffer));
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

        if (header) {
            uint8_t* payload = (uint8_t*)header + sizeof(SP::FrameHeader);

            // Dispatch to TelemetryModel
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
            }
        }
    }
}

void CommLink::processIncomingAck(uint8_t seq, SP::AckPacket* ack) {
    if (_txState == TxState::WAITING_ACK && _jobHead != _jobTail) {
        if (_jobQueue[_jobHead].seqNum == seq) {
            // Success! Advance job queue
            _jobHead = (_jobHead + 1) % MAX_JOBS;
            _txState = TxState::IDLE;

            // Note: We don't need to explicitly zero the buffer; the read pointer (jobHead) just moves.
            // If the queue becomes empty, we can reset pointers to 0 to keep it clean, but circular logic works fine without.
            if (_jobHead == _jobTail) {
                _txWriteIndex = 0; // Optimization: Reset to 0 when empty to reduce fragmentation
            }
        }
    }
}

// --- TX Logic ---

uint16_t CommLink::getFreeSpace() const {
    if (_jobHead == _jobTail) {
        return TX_BUFFER_SIZE - 1; // Empty
    }

    uint16_t readIndex = _jobQueue[_jobHead].bufferOffset;

    if (_txWriteIndex >= readIndex) {
        return TX_BUFFER_SIZE - (_txWriteIndex - readIndex) - 1;
    } else {
        return readIndex - _txWriteIndex - 1;
    }
}

void CommLink::writeToBuffer(const uint8_t* data, uint8_t len, uint16_t offset) {
    if (offset + len <= TX_BUFFER_SIZE) {
        memcpy(&_txRingBuffer[offset], data, len);
    } else {
        uint16_t firstPart = TX_BUFFER_SIZE - offset;
        memcpy(&_txRingBuffer[offset], data, firstPart);
        memcpy(&_txRingBuffer[0], data + firstPart, len - firstPart);
    }
}

void CommLink::sendBufferData(uint16_t offset, uint8_t len) {
    if (!_transport) return;

    if (offset + len <= TX_BUFFER_SIZE) {
        _transport->write(&_txRingBuffer[offset], len);
    } else {
        uint16_t firstPart = TX_BUFFER_SIZE - offset;
        _transport->write(&_txRingBuffer[offset], firstPart);
        _transport->write(&_txRingBuffer[0], len - firstPart);
    }
}

bool CommLink::enqueuePacket(uint8_t msgID, const void* payload, uint8_t payloadLen) {
    // 1. Check Job Queue Space
    uint8_t nextTail = (_jobTail + 1) % MAX_JOBS;
    if (nextTail == _jobHead) return false; // Job queue full

    // 2. Check Buffer Space
    uint16_t totalLen = sizeof(SP::FrameHeader) + payloadLen + 2; // +CRC
    if (getFreeSpace() < totalLen) return false; // Buffer full

    // 3. Assemble Packet in Buffer
    uint16_t startOffset = _txWriteIndex;

    SP::FrameHeader header;
    header.sync1 = SP::PROTOCOL_SYNC_1;
    header.sync2 = SP::PROTOCOL_SYNC_2;
    header.length = payloadLen;
    header.targetID = _model->getShuttleNumber();
    header.seq = _nextSeqNum++;
    header.msgID = msgID;

    // Write Header
    writeToBuffer((uint8_t*)&header, sizeof(header), _txWriteIndex);
    _txWriteIndex = (_txWriteIndex + sizeof(header)) % TX_BUFFER_SIZE;

    // Write Payload
    if (payloadLen > 0) {
        writeToBuffer((uint8_t*)payload, payloadLen, _txWriteIndex);
        _txWriteIndex = (_txWriteIndex + payloadLen) % TX_BUFFER_SIZE;
    }

    // Calculate CRC (Need to read back from buffer to calculate CRC of header+payload!)
    // To avoid reading back, we can calculate CRC on the fly or use a helper.
    // The ProtocolUtils::appendCRC expects a contiguous buffer.
    // Since our buffer might wrap, we can't use appendCRC directly on the ring buffer easily.
    // However, we can calculate CRC while writing? No.
    // Let's iterate the virtual buffer from startOffset to calculate CRC.

    uint16_t crc = 0xFFFF;
    uint16_t idx = startOffset;
    uint16_t lenCheck = sizeof(header) + payloadLen;

    for (uint16_t i = 0; i < lenCheck; i++) {
        uint8_t byte = _txRingBuffer[idx];
        crc = SP::ProtocolUtils::updateCRC16(crc, byte);
        idx = (idx + 1) % TX_BUFFER_SIZE;
    }

    // Write CRC
    uint8_t crcBytes[2];
    crcBytes[0] = (uint8_t)(crc & 0xFF);
    crcBytes[1] = (uint8_t)((crc >> 8) & 0xFF);

    writeToBuffer(crcBytes, 2, _txWriteIndex);
    _txWriteIndex = (_txWriteIndex + 2) % TX_BUFFER_SIZE;

    // 4. Add Job
    TxJob* job = &_jobQueue[_jobTail];
    job->bufferOffset = startOffset;
    job->length = totalLen;
    job->seqNum = header.seq;
    job->retryCount = 0;

    _jobTail = nextTail;

    return true;
}

void CommLink::processTxQueue() {
    if (!_transport) return;

    if (_txState == TxState::IDLE) {
        if (_jobHead != _jobTail) {
            TxJob* job = &_jobQueue[_jobHead];

            if (_transport->availableForWrite() >= job->length) {
                sendBufferData(job->bufferOffset, job->length);
                job->lastTxTime = millis();
                _txState = TxState::WAITING_ACK;
            }
        }
    } else if (_txState == TxState::WAITING_ACK) {
        if (_jobHead != _jobTail) {
            TxJob* job = &_jobQueue[_jobHead];
            if (millis() - job->lastTxTime > 500) {
                if (job->retryCount < 3) {
                    if (_transport->availableForWrite() >= job->length) {
                        job->retryCount++;
                        sendBufferData(job->bufferOffset, job->length);
                        job->lastTxTime = millis();
                    }
                } else {
                    _txState = TxState::TIMEOUT_ERROR;
                }
            }
        } else {
            _txState = TxState::IDLE;
        }
    } else if (_txState == TxState::TIMEOUT_ERROR) {
        // Drop packet
        if (_jobHead != _jobTail) {
            _jobHead = (_jobHead + 1) % MAX_JOBS;
            if (_jobHead == _jobTail) _txWriteIndex = 0;
        }
        _txState = TxState::IDLE;
    }
}

// --- Public Sending API ---

bool CommLink::sendCommand(SP::CommandPacket packet) {
    return enqueuePacket(SP::MSG_COMMAND, &packet, sizeof(packet));
}

bool CommLink::sendRequest(uint8_t msgID) {
    return enqueuePacket(msgID, nullptr, 0);
}

bool CommLink::sendConfigSet(uint8_t paramID, int32_t value) {
    SP::ConfigPacket cfg;
    cfg.paramID = paramID;
    cfg.value = value;
    return enqueuePacket(SP::MSG_CONFIG_SET, &cfg, sizeof(cfg));
}

bool CommLink::sendConfigGet(uint8_t paramID) {
    SP::ConfigPacket cfg;
    cfg.paramID = paramID;
    cfg.value = 0;
    return enqueuePacket(SP::MSG_CONFIG_GET, &cfg, sizeof(cfg));
}

bool CommLink::isQueueFull() const {
    uint8_t nextTail = (_jobTail + 1) % MAX_JOBS;
    return nextTail == _jobHead;
}
