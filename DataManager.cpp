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
    uint32_t timeoutLimit = (_pollingMode == PollingMode::IDLE_KEEPALIVE) ? 6000 : 2000;

    if (_model.isConnected() && (now - _model.getLastRxTime() > timeoutLimit)) {
        _model.setConnected(false);
        LOG_I("DATA", "Connection LOST! Timeout: %u ms", timeoutLimit);
        EventBus::publish(SystemEvent::CONNECTION_LOST);
    }

    if (_pollingMode == PollingMode::CUSTOM_DATA) return;

    if (_isManualMoving) {
        if (now - _manualHeartbeatTimer >= 200) {
            _manualHeartbeatTimer = now;
            if (!_commLink.hasPendingHeartbeat()) {
                 _commLink.sendRequest(SP::MSG_REQ_HEARTBEAT, 0);
            }
        }
    } else {
        uint32_t pollInterval = (_pollingMode == PollingMode::IDLE_KEEPALIVE) ? 2000 : 500;
        if (!_model.isConnected()) pollInterval = 1500;

        if (now - _lastHeartbeatTimer >= pollInterval) {
            _lastHeartbeatTimer = now;
            if (!_commLink.hasPendingHeartbeat()) {
                _commLink.sendRequest(SP::MSG_REQ_HEARTBEAT);
            }
        }
    }
}

bool DataManager::sendCommand(SP::CmdType cmd, int32_t arg1, int32_t arg2) {
    if (!_model.isConnected()) return false; 

    _commLink.clearPendingCommands();
    _lastUserCommandType = cmd;

    uint8_t retries = 1;
    uint16_t timeout = 250;

    switch (cmd) {
        case SP::CMD_STOP:
        case SP::CMD_STOP_MANUAL:
        case SP::CMD_EVACUATE_ON: 
            retries = 4; timeout = 150; break;
        case SP::CMD_LIFT_UP:
        case SP::CMD_LIFT_DOWN:
        case SP::CMD_LOAD:
        case SP::CMD_UNLOAD:
        case SP::CMD_LONG_LOAD:
        case SP::CMD_LONG_UNLOAD:
        case SP::CMD_LONG_UNLOAD_QTY:
        case SP::CMD_COMPACT_F:
        case SP::CMD_COMPACT_R:
        case SP::CMD_CALIBRATE:
        case SP::CMD_HOME:
        case SP::CMD_RESET_ERROR:
        case SP::CMD_SAVE_EEPROM:
        case SP::CMD_SET_DATETIME:
            retries = 2; timeout = 350; break;
        default:
            retries = 1; timeout = 250; break;
    }

    SP::CommandPacket packet;
    packet.cmdType = (uint8_t)cmd;
    packet.arg1 = arg1;
    packet.arg2 = arg2;

    bool sent = _commLink.sendCommand(packet, retries, timeout);
    if (sent) EventBus::publish(SystemEvent::CMD_DISPATCHED);
    return sent;
}

bool DataManager::sendRequest(SP::MsgID msgId) {
    if (_commLink.isWaitingForAck()) return false;
    return _commLink.sendRequest(msgId, 0, 500);
}

SP::CmdType DataManager::getLastUserCommandType() const { return _lastUserCommandType; }

bool DataManager::requestConfig(SP::ConfigParamID paramID) {
    return _commLink.sendConfigGet((uint8_t)paramID);
}
bool DataManager::setConfig(SP::ConfigParamID paramID, int32_t value) {
    return _commLink.sendConfigSet((uint8_t)paramID, value);
}

bool DataManager::requestFullConfig() {
    return _commLink.sendRequest(SP::MSG_CONFIG_SYNC_REQ, 3, 1000);
}
bool DataManager::pushFullConfig(const SP::FullConfigPacket& config) {
    return _commLink.sendFullConfigSync(config);
}

const SP::TelemetryPacket& DataManager::getTelemetry() const { return _model.getTelemetry(); }
const SP::SensorPacket& DataManager::getSensors() const { return _model.getSensors(); }
const SP::StatsPacket& DataManager::getStats() const { return _model.getStats(); }
int32_t DataManager::getConfig(uint8_t index) const { return _model.getConfig(index); }

bool DataManager::hasFullConfig() const { return _model.hasFullConfig(); }
const SP::FullConfigPacket& DataManager::getFullConfig() const { return _model.getFullConfig(); }
void DataManager::invalidateFullConfig() { _model.invalidateFullConfig(); }

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
bool DataManager::isQueueFull() const { return _commLink.isQueueFull(); }