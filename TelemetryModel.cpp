#include "TelemetryModel.h"
#include <Arduino.h>

TelemetryModel::TelemetryModel() : _shuttleNumber(1) {
    memset(&_telemetry, 0, sizeof(_telemetry));
    memset(&_sensors, 0, sizeof(_sensors));
    memset(&_stats, 0, sizeof(_stats));
    memset(_config, 0, sizeof(_config));
}

void TelemetryModel::updateTelemetry(const SP::TelemetryPacket& packet) {
    if (memcmp(&_telemetry, &packet, sizeof(SP::TelemetryPacket)) != 0) {
        memcpy(&_telemetry, &packet, sizeof(SP::TelemetryPacket));
        EventBus::publish(SystemEvent::TELEMETRY_UPDATED);

        // Check for specific events (Low Battery, Error)
        if (_telemetry.batteryCharge < 20) {
            EventBus::publish(SystemEvent::BATTERY_LOW);
        }
        if (_telemetry.errorCode != 0) {
            EventBus::publish(SystemEvent::ERROR_OCCURRED);
        }
    }
}

void TelemetryModel::updateSensors(const SP::SensorPacket& packet) {
    if (memcmp(&_sensors, &packet, sizeof(SP::SensorPacket)) != 0) {
        memcpy(&_sensors, &packet, sizeof(SP::SensorPacket));
        EventBus::publish(SystemEvent::SENSORS_UPDATED);
    }
}

void TelemetryModel::updateStats(const SP::StatsPacket& packet) {
    if (memcmp(&_stats, &packet, sizeof(SP::StatsPacket)) != 0) {
        memcpy(&_stats, &packet, sizeof(SP::StatsPacket));
        EventBus::publish(SystemEvent::STATS_UPDATED);
    }
}

void TelemetryModel::updateConfig(uint8_t paramID, int32_t value) {
    if (paramID < 16) {
        if (_config[paramID] != value) {
            _config[paramID] = value;
            EventBus::publish(SystemEvent::CONFIG_UPDATED);
        }
    }
}

void TelemetryModel::setShuttleNumber(uint8_t id) {
    if (_shuttleNumber != id) {
        _shuttleNumber = id;
        // Could fire SHUTTLE_CHANGED event if needed
    }
}

int32_t TelemetryModel::getConfig(uint8_t index) const {
    if (index < 16) return _config[index];
    return 0;
}
