#pragma once
#include "ITransport.h"
#include "TelemetryModel.h"
#include "ShuttleProtocolTypes.h"
#include "EventBus.h"

class CommLink {
public:
    CommLink(ITransport* transport, TelemetryModel* model);

    void tick();

    bool sendCommand(SP::CommandPacket packet, uint8_t maxRetries = 1, uint16_t ackTimeoutMs = 250);
    bool sendRequest(uint8_t msgID, uint8_t maxRetries = 0, uint16_t ackTimeoutMs = 500);
    bool sendConfigSet(uint8_t paramID, int32_t value);
    bool sendConfigGet(uint8_t paramID);
    bool sendFullConfigSync(const SP::FullConfigPacket& config);

    void clearPendingCommands();

    bool isQueueFull() const;
    bool isWaitingForAck() const;
    bool hasPendingHeartbeat() const;

    enum class TxState { IDLE, WAITING_ACK, TIMEOUT_ERROR };

private:
    ITransport* _transport;
    TelemetryModel* _model;
    SP::ProtocolParser _parser;

    struct TxJob {
        uint8_t packetData[128]; 
        uint8_t length;
        uint8_t seqNum;
        uint32_t lastTxTime;
        uint8_t retryCount;
        uint8_t maxRetries;
        uint16_t ackTimeoutMs;
        bool cancelled;
    };
    static const uint8_t MAX_JOBS = 8;
    TxJob _jobQueue[MAX_JOBS];
    uint8_t _jobHead;
    uint8_t _jobTail;

    TxState _txState;
    uint8_t _nextSeqNum;

    bool enqueuePacket(uint8_t msgID, const void* payload, uint8_t payloadLen, uint8_t maxRetries = 3, uint16_t ackTimeoutMs = 500);
    void processTxQueue();
    void handleRx();
    void processIncomingAck(uint8_t seq, SP::AckPacket* ack);
    bool isUserCommand(uint8_t msgID) const;
};