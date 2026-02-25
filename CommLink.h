#pragma once
#include "ITransport.h"
#include "TelemetryModel.h"
#include "ShuttleProtocolTypes.h"
#include "EventBus.h"

class CommLink {
public:
    CommLink(ITransport* transport, TelemetryModel* model);

    void tick();

    // --- Sending Methods ---
    bool sendCommand(SP::CommandPacket packet);
    bool sendRequest(uint8_t msgID);
    bool sendConfigSet(uint8_t paramID, int32_t value);
    bool sendConfigGet(uint8_t paramID);

    // Clears any pending user commands (preemption)
    void clearPendingCommands();

    bool isQueueFull() const;
    bool isWaitingForAck() const;

    enum class TxState { IDLE, WAITING_ACK, TIMEOUT_ERROR };

private:
    ITransport* _transport;
    TelemetryModel* _model;
    SP::ProtocolParser _parser;

    // --- Zero-Copy TX Ring Buffer ---
    static const uint16_t TX_BUFFER_SIZE = 512;
    uint8_t _txRingBuffer[TX_BUFFER_SIZE];
    uint16_t _txWriteIndex; // Where to write new data

    // --- Job Queue ---
    struct TxJob {
        uint16_t bufferOffset;
        uint8_t length;
        uint8_t seqNum;
        uint32_t lastTxTime;
        uint8_t retryCount;
        bool cancelled;
    };
    static const uint8_t MAX_JOBS = 8;
    TxJob _jobQueue[MAX_JOBS];
    uint8_t _jobHead; // Index of the oldest job (active)
    uint8_t _jobTail; // Index where next job will be added

    // --- State ---
    TxState _txState;
    uint8_t _nextSeqNum;

    // --- Internal Helpers ---
    bool enqueuePacket(uint8_t msgID, const void* payload, uint8_t payloadLen);
    void processTxQueue();
    void handleRx();
    void processIncomingAck(uint8_t seq, SP::AckPacket* ack);
    bool isUserCommand(uint8_t msgID) const;

    // Buffer management
    uint16_t getFreeSpace() const;
    void writeToBuffer(const uint8_t* data, uint8_t len, uint16_t offset);
    void sendBufferData(uint16_t offset, uint8_t len);
};
