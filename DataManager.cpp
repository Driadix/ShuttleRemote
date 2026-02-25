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
      _pollContext(PollContext::NORMAL),
      _lastPollTime(0),
      _lastSensorPollTime(0),
      _lastStatsPollTime(0),
      _manualHeartbeatTimer(0),
      _currentPollInterval(1500),
      _remoteBatteryLevel(0),
      _isCharging(false),
      _radioChannel(0),
      _lastUserCommandType((SP::CmdType)0)
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
            LOG_I("DATA", "Connection LOST! Telemetry timeout.");
            EventBus::publish(SystemEvent::CONNECTION_LOST);
        }
    } else {
        if (_model.getLastRxTime() > 0 && (now - _model.getLastRxTime() < 2000)) {
            _model.setConnected(true);
            LOG_I("DATA", "Connection ESTABLISHED with Shuttle %d", _model.getShuttleNumber());
            EventBus::publish(SystemEvent::CONNECTION_RESTORED);
        }
    }
}

void DataManager::updatePolling() {

    uint32_t now = millis();

    // 1. Dynamic Polling Rate
    uint32_t pollingInterval = _model.isConnected() ? 400 : 1500; // 400ms active, 1.5s discovery

    if (_isManualMoving) {
        // Manual Move Heartbeat Logic (Every 200ms)
        if (now - _manualHeartbeatTimer >= 200) {
            _manualHeartbeatTimer = now; // Always advance timer

            if (!_commLink.hasPendingHeartbeat()) {
                 _commLink.sendRequest(SP::MSG_REQ_HEARTBEAT, 0); // 0 retries
            }
        }
    } else {
        // Standard Polling Logic

        // 1. Heartbeat
        if (now - _lastPollTime >= pollingInterval) {
            // 2. ALWAYS reset the timer, whether we succeed in queueing or not
            _lastPollTime = now;

            // 3. Only queue if we aren't already actively waiting for a heartbeat ACK
            if (!_commLink.hasPendingHeartbeat()) {
                // Send the heartbeat with ZERO retries
                _commLink.sendRequest(SP::MSG_REQ_HEARTBEAT, 0);
                LOG_D("DATA", "Heartbeat queued. Next in %d ms", pollingInterval);
            } else {
                LOG_W("DATA", "Skipped heartbeat queue: Previous heartbeat still in flight.");
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
    LOG_D("DATA", "Dispatching command: MSG_COMMAND (Type: %d) to CommLink", (int)cmd);

    // Preemption: clear previous user commands to enforce "Latest Intent Wins"
    _commLink.clearPendingCommands();
    _lastUserCommandType = cmd;

    // Industrial Latency Strategy
    uint8_t retries = 1;
    uint16_t timeout = 250;

    switch (cmd) {
        // 3. Safety Overrides (Aggressive Spamming)
        case SP::CMD_STOP:
        case SP::CMD_STOP_MANUAL:
        case SP::CMD_EVACUATE_ON: // Assume evacuation is also critical
            retries = 4;
            timeout = 150;
            break;

        // 2. Discrete Actions (Guaranteed Delivery)
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
            retries = 2;
            timeout = 350;
            break;

        // 1. Continuous/Hazardous Movement (Fail Fast)
        case SP::CMD_MOVE_RIGHT_MAN:
        case SP::CMD_MOVE_LEFT_MAN:
        case SP::CMD_MOVE_DIST_R:
        case SP::CMD_MOVE_DIST_F:
            retries = 1;
            timeout = 250;
            break;

        // Default for others (Info requests, etc.)
        default:
            retries = 1;
            timeout = 250;
            break;
    }

    SP::CommandPacket packet;
    packet.cmdType = (uint8_t)cmd;
    packet.arg1 = arg1;
    packet.arg2 = arg2;

    bool sent = _commLink.sendCommand(packet, retries, timeout);
    if (sent) {
        EventBus::publish(SystemEvent::CMD_DISPATCHED);
    }
    return sent;
}

SP::CmdType DataManager::getLastUserCommandType() const {
    return _lastUserCommandType;
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
    LOG_I("DATA", "Config saved. Shuttle: %d", id);
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
