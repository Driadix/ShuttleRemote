#pragma once
#include <Arduino.h>

// Define namespace SP for ShuttleProtocol types
namespace SP {
#include "ShuttleProtocol.h"
}

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

    // --- Dirty Flags (Atomic Check-and-Clear) ---
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

    // --- Internal Types ---
    enum class TxState { IDLE, WAITING_ACK, TIMEOUT_ERROR };

    struct TxJob {
      uint8_t txBuffer[64];
      uint8_t txLength;
      uint8_t seqNum;
      uint32_t lastTxTime;
      uint8_t retryCount;
    };

    // --- Internal Logic ---
    bool queueCommandInternal(SP::CommandPacket packet);
    bool queueRequestInternal(uint8_t msgID);
    bool queueConfigSetInternal(uint8_t paramID, int32_t value);
    bool queueConfigGetInternal(uint8_t paramID);

    void processIncomingAck(uint8_t seq, SP::AckPacket* ack);
    void handleRx();
    void processTxQueue();
    void updatePolling();

    // --- Members ---
    HardwareSerial* _serial;
    SP::ProtocolParser _parser;

    // Data Caches
    SP::TelemetryPacket _telemetry;
    SP::SensorPacket _sensors;
    SP::StatsPacket _stats;
    int32_t _config[16];

    // Dirty Flags
    bool _telemetryDirty;
    bool _sensorsDirty;
    bool _statsDirty;
    bool _configDirty;

    // TX Queue
    static const int TX_QUEUE_SIZE = 5;
    TxJob _txQueue[TX_QUEUE_SIZE];
    uint8_t _txHead;
    uint8_t _txTail;
    TxState _txState;
    uint8_t _nextSeqNum;

    // State / Context
    uint8_t _shuttleNumber;
    bool _isOtaUpdating;
    bool _isManualMoving;
    PollContext _pollContext;

    // Timers
    uint32_t _lastPollTime;
    uint32_t _lastSensorPollTime;
    uint32_t _lastStatsPollTime;
    uint32_t _manualHeartbeatTimer;
    uint32_t _currentPollInterval;
};
