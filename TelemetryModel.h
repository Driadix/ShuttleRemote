#pragma once
#include <stdint.h>
#include <string.h>
#include "EventBus.h"
#include "ShuttleProtocolTypes.h"

class TelemetryModel {
public:
    TelemetryModel();

    // --- Setters (Trigger Events if changed) ---
    void updateTelemetry(const SP::TelemetryPacket& packet);
    void updateSensors(const SP::SensorPacket& packet);
    void updateStats(const SP::StatsPacket& packet);
    void updateConfig(uint8_t paramID, int32_t value);
    void setShuttleNumber(uint8_t id);

    // --- Getters ---
    const SP::TelemetryPacket& getTelemetry() const { return _telemetry; }
    const SP::SensorPacket& getSensors() const { return _sensors; }
    const SP::StatsPacket& getStats() const { return _stats; }
    int32_t getConfig(uint8_t index) const;
    uint8_t getShuttleNumber() const { return _shuttleNumber; }

    bool isConnected() const { return _isConnected; }
    uint32_t getLastRxTime() const { return _lastRxTime; }
    void setConnected(bool connected) { _isConnected = connected; }

private:
    SP::TelemetryPacket _telemetry;
    SP::SensorPacket _sensors;
    SP::StatsPacket _stats;
    int32_t _config[16];
    uint8_t _shuttleNumber;

    uint32_t _lastRxTime;
    bool _isConnected;
};
