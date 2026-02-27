#include "DataManager.h"
#include <Preferences.h>
#include "Logger.h"
#include "DebugUtils.h"

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
      _pollingMode(PollingMode::ACTIVE_TELEMETRY),
      _lastHeartbeatTimer(0),
      _manualHeartbeatTimer(0),
      _remoteBatteryLevel(0),
      _isCharging(false),
      _radioChannel(0),
      _lastUserCommandType((SP::CmdType)0)
{
}

void DataManager::init(HardwareSerial* serial, uint8_t shuttleNum) {
    _transport.setSerial(serial);
    _model.setTargetShuttleID(shuttleNum);
    EventBus::subscribe(this);
}

void DataManager::onEvent(SystemEvent event) {
    if (event == SystemEvent::COMM_RX_ACTIVITY) {
        if (!_model.isConnected()) {
             _model.setConnected(true);
             LOG_I("DATA", "Connection RESTORED (Activity Detected)");
             EventBus::publish(SystemEvent::CONNECTION_RESTORED);
        }
    }
}

void DataManager::tick() {
    if (_isOtaUpdating) return;

    _commLink.tick();

    uint32_t now = millis();
    
    // 1. Connection Watchdog
    uint32_t timeoutLimit = 4000; 
    if (_model.isConnected() && (now - _model.getLastRxTime() > timeoutLimit)) {
        _model.setConnected(false);
        LOG_I("DATA", "Connection LOST! Timeout");
        EventBus::publish(SystemEvent::CONNECTION_LOST);
    }

    // 2. Dynamic Polling State Machine
    if (_isManualMoving) {
        if (now - _manualHeartbeatTimer >= 200) {
            _manualHeartbeatTimer = now;
            _commLink.sendRequest(SP::MSG_REQ_HEARTBEAT);
        }
    } else {
        uint32_t heartbeatInterval;
        if (!_model.isConnected()) {
            heartbeatInterval = 1500; // Hunting for shuttle
        } else if (_pollingMode == PollingMode::ACTIVE_TELEMETRY) {
            heartbeatInterval = 500;  // Fast updates on Dashboard
        } else if (_pollingMode == PollingMode::IDLE_KEEPALIVE) {
            heartbeatInterval = 1500; // Slow updates in menus
        } else { // CUSTOM_DATA
            heartbeatInterval = 3000; // Let other requests dominate
        }

        if (now - _lastHeartbeatTimer >= heartbeatInterval) {
            _lastHeartbeatTimer = now;
            _commLink.sendRequest(SP::MSG_REQ_HEARTBEAT);
        }
    }
}

bool DataManager::sendCommand(SP::CmdType cmd, int32_t arg) {
    if (!_model.isConnected()) return false; 

    _lastUserCommandType = cmd;

    uint8_t retries = 0; // Stateless fire-and-forget for actions!
    uint16_t timeout = 1000;

    switch (cmd) {
        case SP::CMD_CALIBRATE:
        case SP::CMD_SAVE_EEPROM:
            retries = 1; // Only invisible system states get a retry
            timeout = 1500;
            break;
        default: break;
    }

    bool needsArg = (cmd == SP::CMD_MOVE_DIST_R || 
                     cmd == SP::CMD_MOVE_DIST_F || 
                     cmd == SP::CMD_LONG_UNLOAD_QTY);

    bool sent = false;
    if (needsArg) {
        SP::ParamCmdPacket packet = { arg, (uint8_t)cmd };
        sent = _commLink.sendCommandWithArg(packet, retries, timeout);
    } else {
        SP::SimpleCmdPacket packet = { (uint8_t)cmd };
        sent = _commLink.sendSimpleCommand(packet, retries, timeout);
    }
    
    if (sent) EventBus::publish(SystemEvent::CMD_DISPATCHED);
    return sent;
}

bool DataManager::sendRequest(SP::MsgID msgId) {
    // Fire and forget request. Does not queue, does not wait for ACK.
    return _commLink.sendRequest(msgId);
}

SP::CmdType DataManager::getLastUserCommandType() const { return _lastUserCommandType; }

bool DataManager::requestConfig(SP::ConfigParamID paramID) { return _commLink.sendConfigGet((uint8_t)paramID); }
bool DataManager::setConfig(SP::ConfigParamID paramID, int32_t value) { return _commLink.sendConfigSet((uint8_t)paramID, value); }
bool DataManager::requestFullConfig() { return sendRequest(SP::MSG_CONFIG_SYNC_REQ); }
bool DataManager::pushFullConfig(const SP::FullConfigPacket& config) { return _commLink.sendFullConfigSync(config); }

const SP::TelemetryPacket& DataManager::getTelemetry() const { return _model.getTelemetry(); }
const SP::SensorPacket& DataManager::getSensors() const { return _model.getSensors(); }
const SP::StatsPacket& DataManager::getStats() const { return _model.getStats(); }
int32_t DataManager::getConfig(uint8_t index) const { return _model.getConfig(index); }

bool DataManager::hasFullConfig() const { return _model.hasFullConfig(); }
const SP::FullConfigPacket& DataManager::getFullConfig() const { return _model.getFullConfig(); }
void DataManager::invalidateFullConfig() { _model.invalidateFullConfig(); }

bool DataManager::hasValidStats() const { return _model.hasValidStats(); }
void DataManager::invalidateStats() { _model.invalidateStats(); }

bool DataManager::hasValidSensors() const { return _model.hasValidSensors(); }
void DataManager::invalidateSensors() { _model.invalidateSensors(); }

uint8_t DataManager::getTargetShuttleID() const { return _model.getTargetShuttleID(); }
void DataManager::setPollingMode(PollingMode mode) { _pollingMode = mode; }
void DataManager::setTargetShuttleID(uint8_t id) { _model.setTargetShuttleID(id); }

void DataManager::saveLocalShuttleNumber(uint8_t id) {
    Preferences prefs;
    prefs.begin("pult_cfg", false);
    prefs.putUInt("sht_num", id);
    prefs.end();
    setTargetShuttleID(id);
}

void DataManager::setOtaUpdating(bool isUpdating) { _isOtaUpdating = isUpdating; }

void DataManager::setManualMoveMode(bool isMoving) {
    if (_isManualMoving != isMoving) {
        _isManualMoving = isMoving;
        if (isMoving) _manualHeartbeatTimer = millis();
    }
}

void DataManager::setRemoteBatteryLevel(int level, bool isCharging) {
    if (_remoteBatteryLevel != level || _isCharging != isCharging) {
        _remoteBatteryLevel = level;
        _isCharging = isCharging;
        EventBus::publish(SystemEvent::LOCAL_BATT_UPDATED);
    }
}

void DataManager::setRadioChannel(uint8_t ch) { _radioChannel = ch; }
int DataManager::getRemoteBatteryLevel() const { return _remoteBatteryLevel; }
uint8_t DataManager::getRadioChannel() const { return _radioChannel; }
bool DataManager::isConnected() const { return _model.isConnected(); }
bool DataManager::isWaitingForAck() const { return _commLink.isWaitingForAck(); }
bool DataManager::isCharging() const { return _isCharging; }