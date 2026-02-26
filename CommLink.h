#pragma once
#include "ITransport.h"
#include "TelemetryModel.h"
#include "ShuttleProtocolTypes.h"
#include "EventBus.h"

class CommLink {
public:
    CommLink(ITransport* transport, TelemetryModel* model);

    void tick();

    // Sends command with smart throttle. Replaces any pending tracked transmission.
    bool sendCommand(SP::CommandPacket packet, uint8_t maxRetries = 0, uint16_t ackTimeoutMs = 1000);
    
    // Fire-and-forget. Sends immediately, doesn't wait for ACK.
    bool sendRequest(uint8_t msgID); 
    
    // Tracked settings updates
    bool sendConfigSet(uint8_t paramID, int32_t value);
    bool sendConfigGet(uint8_t paramID);
    bool sendFullConfigSync(const SP::FullConfigPacket& config);

    void clearPendingCommands();
    bool isWaitingForAck() const;

    enum class TxState { IDLE, WAITING_ACK, TIMEOUT_ERROR };

private:
    ITransport* _transport;
    TelemetryModel* _model;
    SP::ProtocolParser _parser;

    // Single-Slot Preemptive Tracker
    uint8_t _trackedBuffer[128];
    uint16_t _trackedLength;
    uint8_t _trackedSeq;
    uint8_t _trackedMaxRetries;
    uint8_t _trackedRetries;
    uint16_t _trackedTimeout;
    uint32_t _trackedTxTime;

    // Command Throttling
    uint32_t _lastCmdTxTime;
    uint8_t _lastSentCmdType;

    TxState _txState;
    uint8_t _nextSeqNum;

    void transmitRawPacket(uint8_t msgID, const void* payload, uint8_t payloadLen, uint8_t* outBuffer, uint16_t& outLen);
    bool trackAndTransmit(uint8_t msgID, const void* payload, uint8_t payloadLen, uint8_t maxRetries, uint16_t timeout);
    
    void processTxQueue();
    void handleRx();
    void processIncomingAck(uint8_t seq, SP::AckPacket* ack);
    bool isUserCommand(uint8_t msgID) const;
};