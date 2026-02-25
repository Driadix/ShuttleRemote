#include "DataManager.h"
#include <Preferences.h>

DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

DataManager::DataManager()
    : _transport(nullptr),
      _model(),
      _commLink(&_transport, &_model),
      _isOtaUpdating(false),
      _isManualMoving(false),
      _pollContext(PollContext::NORMAL),
      _lastPollTime(0),
      _lastSensorPollTime(0),
      _lastStatsPollTime(0),
      _manualHeartbeatTimer(0),
      _currentPollInterval(1500),
      _remoteBatteryLevel(0),
      _isCharging(false),
      _radioChannel(0)
{
}

void DataManager::init(HardwareSerial* serial, uint8_t shuttleNum) {
    _transport.setSerial(serial);
    _model.setShuttleNumber(shuttleNum);
}

void DataManager::tick() {
    if (_isOtaUpdating) return;

    _commLink.tick();
    updatePolling();

    // Connection Watchdog
    uint32_t now = millis();
    bool currentlyConnected = _model.isConnected();
    if (currentlyConnected) {
        if (now - _model.getLastRxTime() > 2000) {
            _model.setConnected(false);
            EventBus::publish(SystemEvent::CONNECTION_LOST);
        }
    } else {
        if (_model.getLastRxTime() > 0 && (now - _model.getLastRxTime() < 2000)) {
            _model.setConnected(true);
            EventBus::publish(SystemEvent::CONNECTION_RESTORED);
        }
    }
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
            if (_commLink.sendRequest(SP::MSG_REQ_HEARTBEAT)) {
                 _manualHeartbeatTimer = now;
            }
        }
    } else {
        // Standard Polling Logic

        // 1. Heartbeat
        if (now - _lastPollTime >= _currentPollInterval) {
            if (_commLink.sendRequest(SP::MSG_REQ_HEARTBEAT)) {
                _lastPollTime = now;
            }
        }

        // 2. Sensors (if in Debug Context)
        if (_pollContext == PollContext::DEBUG_SENSORS) {
            if (now - _lastSensorPollTime >= 300) {
                if (_commLink.sendRequest(SP::MSG_REQ_SENSORS)) {
                    _lastSensorPollTime = now;
                }
            }
        }

        // 3. Stats (if in Stats Context)
        if (_pollContext == PollContext::STATS_VIEW) {
            if (now - _lastStatsPollTime >= 2000) {
                if (_commLink.sendRequest(SP::MSG_REQ_STATS)) {
                    _lastStatsPollTime = now;
                }
            }
        }
    }
}

// --- Public API ---

bool DataManager::sendCommand(SP::CmdType cmd, int32_t arg1, int32_t arg2) {
    SP::CommandPacket packet;
    packet.cmdType = (uint8_t)cmd;
    packet.arg1 = arg1;
    packet.arg2 = arg2;
    return _commLink.sendCommand(packet);
}

bool DataManager::requestConfig(SP::ConfigParamID paramID) {
    return _commLink.sendConfigGet((uint8_t)paramID);
}

bool DataManager::setConfig(SP::ConfigParamID paramID, int32_t value) {
    return _commLink.sendConfigSet((uint8_t)paramID, value);
}

// --- Getters ---

const SP::TelemetryPacket& DataManager::getTelemetry() const { return _model.getTelemetry(); }
const SP::SensorPacket& DataManager::getSensors() const { return _model.getSensors(); }
const SP::StatsPacket& DataManager::getStats() const { return _model.getStats(); }
int32_t DataManager::getConfig(uint8_t index) const { return _model.getConfig(index); }
uint8_t DataManager::getShuttleNumber() const { return _model.getShuttleNumber(); }

// --- Setters ---

void DataManager::setPollContext(PollContext ctx) {
    _pollContext = ctx;
}

void DataManager::setShuttleNumber(uint8_t id) {
    _model.setShuttleNumber(id);
}

void DataManager::saveLocalShuttleNumber(uint8_t id) {
    Preferences prefs;
    prefs.begin("pult_cfg", false);
    prefs.putUInt("sht_num", id);
    prefs.end();
    setShuttleNumber(id);
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

void DataManager::setRemoteBatteryLevel(int level, bool isCharging) {
    if (_remoteBatteryLevel != level || _isCharging != isCharging) {
        _remoteBatteryLevel = level;
        _isCharging = isCharging;
        EventBus::publish(SystemEvent::LOCAL_BATT_UPDATED);
    }
}

void DataManager::setRadioChannel(uint8_t ch) {
    _radioChannel = ch;
}

int DataManager::getRemoteBatteryLevel() const {
    return _remoteBatteryLevel;
}

uint8_t DataManager::getRadioChannel() const {
    return _radioChannel;
}

bool DataManager::isConnected() const {
    return _model.isConnected();
}

bool DataManager::isWaitingForAck() const {
    return _commLink.isWaitingForAck();
}

bool DataManager::isCharging() const {
    return _isCharging;
}

bool DataManager::isQueueFull() const {
    return _commLink.isQueueFull();
}
