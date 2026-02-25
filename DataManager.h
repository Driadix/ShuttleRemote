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

    // --- Dirty Flags (Legacy / Event Bridge) ---
    // Deprecated: Screens should use EventBus, but kept for compatibility during migration
    bool consumeTelemetryDirtyFlag();
    bool consumeSensorsDirtyFlag();
    bool consumeStatsDirtyFlag();
    bool consumeConfigDirtyFlag();

    // --- State Setters ---
    void setPollContext(PollContext ctx);
    void setShuttleNumber(uint8_t id);
    void setOtaUpdating(bool isUpdating);
    void setManualMoveMode(bool isMoving);

    // --- Utils ---
    bool isQueueFull() const;

private:
    DataManager();
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;

    // --- Internal Logic ---
    void updatePolling();

    // --- Components ---
    UartTransport* _transport;
    CommLink* _commLink;
    TelemetryModel* _model;

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

    // --- Legacy Dirty Flags (Managed by EventListener internally if needed, or just removed if screens updated) ---
    // To support legacy consume*DirtyFlag, we can subscribe DataManager to EventBus or just query Model?
    // Actually, the simplest way is to have DataManager listen to EventBus and set these flags.
    // Or, update TelemetryModel to have dirty flags? No, model should be pure.
    // I'll make DataManager an EventListener for backward compatibility!
    // But DataManager is a singleton, so it can subscribe itself.

    // Actually, I'll implement EventListener interface privately to update flags.
    // But since I'm updating screens in step 4, maybe I can remove these?
    // The plan says "Keep the public API backward compatible".
    // So I must keep consumeTelemetryDirtyFlag.
    // I will implement a private EventListener for DataManager or just set flags when calling updateModel?
    // No, Model updates via CommLink. DataManager doesn't know when update happens unless it listens.
    // So DataManager needs to subscribe to EventBus.

    class DataManagerListener : public EventListener {
    public:
        DataManagerListener(DataManager* parent) : _parent(parent) {}
        void onEvent(SystemEvent event) override {
            if (event == SystemEvent::TELEMETRY_UPDATED) _parent->_telemetryDirty = true;
            else if (event == SystemEvent::SENSORS_UPDATED) _parent->_sensorsDirty = true;
            else if (event == SystemEvent::STATS_UPDATED) _parent->_statsDirty = true;
            else if (event == SystemEvent::CONFIG_UPDATED) _parent->_configDirty = true;
        }
    private:
        DataManager* _parent;
    };

    friend class DataManagerListener;
    DataManagerListener* _internalListener;
    bool _telemetryDirty;
    bool _sensorsDirty;
    bool _statsDirty;
    bool _configDirty;
};
