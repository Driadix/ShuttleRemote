#pragma once
#include <Arduino.h>
#include "CommLink.h"
#include "TelemetryModel.h"
#include "UartTransport.h"
#include "ShuttleProtocolTypes.h"

class DataManager {
public:
    enum class PollContext {
        NORMAL,
        MAIN_DASHBOARD, // Fast updates (400ms)
        DEBUG_SENSORS,  // Sensors polling
        STATS_VIEW      // Stats polling
    };

    static DataManager& getInstance();

    // Initialization
    void init(HardwareSerial* serial, uint8_t shuttleNum);

    // Main Loop Tick
    void tick();

    // --- Command Gateway ---
    bool sendCommand(SP::CmdType cmd, int32_t arg1 = 0, int32_t arg2 = 0);
    bool requestConfig(SP::ConfigParamID paramID);
    bool setConfig(SP::ConfigParamID paramID, int32_t value);

    // --- Data Getters (Read-Only) ---
    const SP::TelemetryPacket& getTelemetry() const;
    const SP::SensorPacket& getSensors() const;
    const SP::StatsPacket& getStats() const;
    int32_t getConfig(uint8_t index) const;
    uint8_t getShuttleNumber() const;
    int getRemoteBatteryLevel() const;
    uint8_t getRadioChannel() const;
    bool isConnected() const;
    bool isWaitingForAck() const;
    bool isCharging() const;

    // --- State Setters ---
    void setPollContext(PollContext ctx);
    void setShuttleNumber(uint8_t id);
    void saveLocalShuttleNumber(uint8_t id);
    void setOtaUpdating(bool isUpdating);
    void setManualMoveMode(bool isMoving);
    void setRemoteBatteryLevel(int level, bool isCharging);
    void setRadioChannel(uint8_t ch);

    // --- Utils ---
    bool isQueueFull() const;

private:
    DataManager();
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;

    // --- Internal Logic ---
    void updatePolling();

    // --- Components ---
    UartTransport _transport;
    TelemetryModel _model;
    CommLink _commLink;

    // --- State / Context ---
    bool _isOtaUpdating;
    bool _isManualMoving;
    PollContext _pollContext;

    // Timers
    uint32_t _lastPollTime;
    uint32_t _lastSensorPollTime;
    uint32_t _lastStatsPollTime;
    uint32_t _manualHeartbeatTimer;
    uint32_t _currentPollInterval;

    int _remoteBatteryLevel;
    bool _isCharging;
    uint8_t _radioChannel;
};
