#pragma once
#include <Arduino.h>
#include "CommLink.h"
#include "TelemetryModel.h"
#include "UartTransport.h"
#include "ShuttleProtocolTypes.h"
#include "EventBus.h"

class DataManager : public EventListener {
public:
    enum class PollingMode {
        ACTIVE_TELEMETRY,
        CUSTOM_DATA,      
        IDLE_KEEPALIVE    
    };

    static DataManager& getInstance();

    void init(HardwareSerial* serial, uint8_t shuttleNum);
    void tick();
    virtual void onEvent(SystemEvent event) override;

    bool sendCommand(SP::CmdType cmd, int32_t arg1 = 0, int32_t arg2 = 0);
    bool sendRequest(SP::MsgID msgId);
    
    bool requestConfig(SP::ConfigParamID paramID);
    bool setConfig(SP::ConfigParamID paramID, int32_t value);

    bool requestFullConfig();
    bool pushFullConfig(const SP::FullConfigPacket& config);

    SP::CmdType getLastUserCommandType() const;

    const SP::TelemetryPacket& getTelemetry() const;
    const SP::SensorPacket& getSensors() const;
    const SP::StatsPacket& getStats() const;
    int32_t getConfig(uint8_t index) const;

    bool hasFullConfig() const;
    const SP::FullConfigPacket& getFullConfig() const;
    void invalidateFullConfig();
    
    bool hasValidStats() const;
    void invalidateStats();
    
    bool hasValidSensors() const;
    void invalidateSensors();
    
    uint8_t getTargetShuttleID() const;
    int getRemoteBatteryLevel() const;
    uint8_t getRadioChannel() const;
    bool isConnected() const;
    bool isWaitingForAck() const;
    bool isCharging() const;

    void setPollingMode(PollingMode mode);
    void setTargetShuttleID(uint8_t id);
    void saveLocalShuttleNumber(uint8_t id);
    void setOtaUpdating(bool isUpdating);
    void setManualMoveMode(bool isMoving);
    void setRemoteBatteryLevel(int level, bool isCharging);
    void setRadioChannel(uint8_t ch);

private:
    DataManager();
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;

    UartTransport _transport;
    TelemetryModel _model;
    CommLink _commLink;

    bool _isOtaUpdating;
    bool _isManualMoving;
    PollingMode _pollingMode;

    uint32_t _lastHeartbeatTimer;
    uint32_t _manualHeartbeatTimer;

    int _remoteBatteryLevel;
    bool _isCharging;
    uint8_t _radioChannel;
    SP::CmdType _lastUserCommandType;
};