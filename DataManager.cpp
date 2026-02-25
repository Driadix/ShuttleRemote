#include "DataManager.h"

DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

DataManager::DataManager()
    : _serial(nullptr),
      _telemetryDirty(false),
      _sensorsDirty(false),
      _statsDirty(false),
      _configDirty(false),
      _txHead(0),
      _txTail(0),
      _txState(TxState::IDLE),
      _nextSeqNum(0),
      _shuttleNumber(1),
      _isOtaUpdating(false),
      _isManualMoving(false),
      _pollContext(PollContext::NORMAL),
      _lastPollTime(0),
      _lastSensorPollTime(0),
      _lastStatsPollTime(0),
      _manualHeartbeatTimer(0),
      _currentPollInterval(1500)
{
    memset(&_telemetry, 0, sizeof(_telemetry));
    memset(&_sensors, 0, sizeof(_sensors));
    memset(&_stats, 0, sizeof(_stats));
    memset(_config, 0, sizeof(_config));
    memset(_txQueue, 0, sizeof(_txQueue));
}

void DataManager::init(HardwareSerial* serial, uint8_t shuttleNum) {
    _serial = serial;
    _shuttleNumber = shuttleNum;

    // Initialize default config values if needed, or rely on updates
}

void DataManager::tick() {
    if (_isOtaUpdating) return;

    handleRx();
    processTxQueue();
    updatePolling();
}

void DataManager::updatePolling() {
    uint32_t now = millis();

    // Determine base poll interval
    if (_pollContext == PollContext::MAIN_DASHBOARD) {
        _currentPollInterval = 400;
    } else {
        _currentPollInterval = 1500;
    }

    if (_isManualMoving) {
        // Manual Move Heartbeat Logic (Every 200ms)
        if (now - _manualHeartbeatTimer >= 200) {
            if (queueRequestInternal(SP::MSG_REQ_HEARTBEAT)) {
                 _manualHeartbeatTimer = now;
            }
        }
    } else {
        // Standard Polling Logic

        // 1. Heartbeat
        if (now - _lastPollTime >= _currentPollInterval) {
            if (queueRequestInternal(SP::MSG_REQ_HEARTBEAT)) {
                _lastPollTime = now;
            }
        }

        // 2. Sensors (if in Debug Context)
        if (_pollContext == PollContext::DEBUG_SENSORS) {
            if (now - _lastSensorPollTime >= 300) {
                if (queueRequestInternal(SP::MSG_REQ_SENSORS)) {
                    _lastSensorPollTime = now;
                }
            }
        }

        // 3. Stats (if in Stats Context)
        if (_pollContext == PollContext::STATS_VIEW) {
            if (now - _lastStatsPollTime >= 2000) {
                if (queueRequestInternal(SP::MSG_REQ_STATS)) {
                    _lastStatsPollTime = now;
                }
            }
        }
    }
}

// --- TX Queue Management ---

void DataManager::processTxQueue() {
    if (_isOtaUpdating || !_serial) return;

    if (_txState == TxState::IDLE) {
        if (_txHead != _txTail) {
            TxJob* job = &_txQueue[_txHead];

            if (_serial->availableForWrite() >= job->txLength) {
                _serial->write(job->txBuffer, job->txLength);
                job->lastTxTime = millis();
                _txState = TxState::WAITING_ACK;
            }
        }
    } else if (_txState == TxState::WAITING_ACK) {
        if (_txHead != _txTail) {
            TxJob* job = &_txQueue[_txHead];
            if (millis() - job->lastTxTime > 500) {
                if (job->retryCount < 3) {
                    if (_serial->availableForWrite() >= job->txLength) {
                        job->retryCount++;
                        _serial->write(job->txBuffer, job->txLength);
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
        if (_txHead != _txTail) {
            _txHead = (_txHead + 1) % TX_QUEUE_SIZE;
        }
        _txState = TxState::IDLE;
    }
}

void DataManager::processIncomingAck(uint8_t seq, SP::AckPacket* ack) {
    if (_txState == TxState::WAITING_ACK && _txHead != _txTail) {
        if (_txQueue[_txHead].seqNum == seq) {
            _txHead = (_txHead + 1) % TX_QUEUE_SIZE;
            _txState = TxState::IDLE;
        }
    }
}

void DataManager::handleRx() {
    if (_isOtaUpdating || !_serial) return;

    while (_serial->available()) {
        uint8_t byte = _serial->read();
        SP::FrameHeader* header = _parser.feed(byte);

        if (header) {
            uint8_t* payload = (uint8_t*)header + sizeof(SP::FrameHeader);

            switch (header->msgID) {
                case SP::MSG_ACK:
                    processIncomingAck(header->seq, (SP::AckPacket*)payload);
                    break;

                case SP::MSG_HEARTBEAT:
                    if (header->length == sizeof(SP::TelemetryPacket)) {
                        if (memcmp(&_telemetry, payload, sizeof(SP::TelemetryPacket)) != 0) {
                            memcpy(&_telemetry, payload, sizeof(SP::TelemetryPacket));
                            _telemetryDirty = true;
                        }
                    }
                    break;

                case SP::MSG_SENSORS:
                    if (header->length == sizeof(SP::SensorPacket)) {
                        if (memcmp(&_sensors, payload, sizeof(SP::SensorPacket)) != 0) {
                            memcpy(&_sensors, payload, sizeof(SP::SensorPacket));
                            _sensorsDirty = true;
                        }
                    }
                    break;

                case SP::MSG_STATS:
                    if (header->length == sizeof(SP::StatsPacket)) {
                        if (memcmp(&_stats, payload, sizeof(SP::StatsPacket)) != 0) {
                            memcpy(&_stats, payload, sizeof(SP::StatsPacket));
                            _statsDirty = true;
                        }
                    }
                    break;

                case SP::MSG_CONFIG_REP:
                    if (header->length == sizeof(SP::ConfigPacket)) {
                        SP::ConfigPacket* pkt = (SP::ConfigPacket*)payload;
                        if (pkt->paramID < 16) {
                            _config[pkt->paramID] = pkt->value;
                            _configDirty = true; // Use specific logic if needed
                        }
                    }
                    break;
            }
        }
    }
}

// --- Queue Helpers ---

bool DataManager::isQueueFull() const {
     uint8_t nextTail = (_txTail + 1) % TX_QUEUE_SIZE;
     return nextTail == _txHead;
}

bool DataManager::queueCommandInternal(SP::CommandPacket packet) {
    uint8_t nextTail = (_txTail + 1) % TX_QUEUE_SIZE;
    if (nextTail != _txHead) {
        TxJob* job = &_txQueue[_txTail];
        job->seqNum = _nextSeqNum++;
        job->retryCount = 0;

        SP::FrameHeader* header = (SP::FrameHeader*)job->txBuffer;
        header->sync1 = SP::PROTOCOL_SYNC_1;
        header->sync2 = SP::PROTOCOL_SYNC_2;
        header->length = sizeof(SP::CommandPacket);
        header->targetID = _shuttleNumber;
        header->seq = job->seqNum;
        header->msgID = SP::MSG_COMMAND;

        memcpy(job->txBuffer + sizeof(SP::FrameHeader), &packet, sizeof(SP::CommandPacket));
        SP::ProtocolUtils::appendCRC(job->txBuffer, sizeof(SP::FrameHeader) + sizeof(SP::CommandPacket));
        job->txLength = sizeof(SP::FrameHeader) + sizeof(SP::CommandPacket) + 2;

        _txTail = nextTail;
        return true;
    }
    return false;
}

bool DataManager::queueRequestInternal(uint8_t msgID) {
    uint8_t nextTail = (_txTail + 1) % TX_QUEUE_SIZE;
    if (nextTail != _txHead) {
        TxJob* job = &_txQueue[_txTail];
        job->seqNum = _nextSeqNum++;
        job->retryCount = 0;

        SP::FrameHeader* header = (SP::FrameHeader*)job->txBuffer;
        header->sync1 = SP::PROTOCOL_SYNC_1;
        header->sync2 = SP::PROTOCOL_SYNC_2;
        header->length = 0;
        header->targetID = _shuttleNumber;
        header->seq = job->seqNum;
        header->msgID = msgID;

        SP::ProtocolUtils::appendCRC(job->txBuffer, sizeof(SP::FrameHeader));
        job->txLength = sizeof(SP::FrameHeader) + 2;

        _txTail = nextTail;
        return true;
    }
    return false;
}

bool DataManager::queueConfigSetInternal(uint8_t paramID, int32_t value) {
    uint8_t nextTail = (_txTail + 1) % TX_QUEUE_SIZE;
    if (nextTail != _txHead) {
        TxJob* job = &_txQueue[_txTail];
        job->seqNum = _nextSeqNum++;
        job->retryCount = 0;

        SP::ConfigPacket cfg;
        cfg.paramID = paramID;
        cfg.value = value;

        SP::FrameHeader* header = (SP::FrameHeader*)job->txBuffer;
        header->sync1 = SP::PROTOCOL_SYNC_1;
        header->sync2 = SP::PROTOCOL_SYNC_2;
        header->length = sizeof(SP::ConfigPacket);
        header->targetID = _shuttleNumber;
        header->seq = job->seqNum;
        header->msgID = SP::MSG_CONFIG_SET;

        memcpy(job->txBuffer + sizeof(SP::FrameHeader), &cfg, sizeof(SP::ConfigPacket));
        SP::ProtocolUtils::appendCRC(job->txBuffer, sizeof(SP::FrameHeader) + sizeof(SP::ConfigPacket));
        job->txLength = sizeof(SP::FrameHeader) + sizeof(SP::ConfigPacket) + 2;

        _txTail = nextTail;
        return true;
    }
    return false;
}

bool DataManager::queueConfigGetInternal(uint8_t paramID) {
    uint8_t nextTail = (_txTail + 1) % TX_QUEUE_SIZE;
    if (nextTail != _txHead) {
        TxJob* job = &_txQueue[_txTail];
        job->seqNum = _nextSeqNum++;
        job->retryCount = 0;

        SP::ConfigPacket cfg;
        cfg.paramID = paramID;
        cfg.value = 0; // Value ignored for GET

        SP::FrameHeader* header = (SP::FrameHeader*)job->txBuffer;
        header->sync1 = SP::PROTOCOL_SYNC_1;
        header->sync2 = SP::PROTOCOL_SYNC_2;
        header->length = sizeof(SP::ConfigPacket);
        header->targetID = _shuttleNumber;
        header->seq = job->seqNum;
        header->msgID = SP::MSG_CONFIG_GET;

        memcpy(job->txBuffer + sizeof(SP::FrameHeader), &cfg, sizeof(SP::ConfigPacket));
        SP::ProtocolUtils::appendCRC(job->txBuffer, sizeof(SP::FrameHeader) + sizeof(SP::ConfigPacket));
        job->txLength = sizeof(SP::FrameHeader) + sizeof(SP::ConfigPacket) + 2;

        _txTail = nextTail;
        return true;
    }
    return false;
}

// --- Public API ---

bool DataManager::sendCommand(SP::CmdType cmd, int32_t arg1, int32_t arg2) {
    SP::CommandPacket packet;
    packet.cmdType = (uint8_t)cmd;
    packet.arg1 = arg1;
    packet.arg2 = arg2;
    return queueCommandInternal(packet);
}

bool DataManager::requestConfig(SP::ConfigParamID paramID) {
    return queueConfigGetInternal((uint8_t)paramID);
}

bool DataManager::setConfig(SP::ConfigParamID paramID, int32_t value) {
    return queueConfigSetInternal((uint8_t)paramID, value);
}

// --- Getters ---

const SP::TelemetryPacket& DataManager::getTelemetry() const { return _telemetry; }
const SP::SensorPacket& DataManager::getSensors() const { return _sensors; }
const SP::StatsPacket& DataManager::getStats() const { return _stats; }
int32_t DataManager::getConfig(uint8_t index) const {
    if (index < 16) return _config[index];
    return 0;
}

bool DataManager::consumeTelemetryDirtyFlag() {
    bool dirty = _telemetryDirty;
    _telemetryDirty = false;
    return dirty;
}
bool DataManager::consumeSensorsDirtyFlag() {
    bool dirty = _sensorsDirty;
    _sensorsDirty = false;
    return dirty;
}
bool DataManager::consumeStatsDirtyFlag() {
    bool dirty = _statsDirty;
    _statsDirty = false;
    return dirty;
}
bool DataManager::consumeConfigDirtyFlag() {
    bool dirty = _configDirty;
    _configDirty = false;
    return dirty;
}

uint8_t DataManager::getShuttleNumber() const {
    return _shuttleNumber;
}

// --- Setters ---

void DataManager::setPollContext(PollContext ctx) {
    _pollContext = ctx;
}

void DataManager::setShuttleNumber(uint8_t id) {
    _shuttleNumber = id;
}

void DataManager::setOtaUpdating(bool isUpdating) {
    _isOtaUpdating = isUpdating;
}

void DataManager::setManualMoveMode(bool isMoving) {
    if (_isManualMoving != isMoving) {
        _isManualMoving = isMoving;
        if (isMoving) {
            _manualHeartbeatTimer = millis();
        }
    }
}
