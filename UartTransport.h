#pragma once
#include "ITransport.h"
#include <Arduino.h>

class UartTransport : public ITransport {
public:
    UartTransport(HardwareSerial* serial);

    virtual int available() override;
    virtual int read() override;
    virtual size_t write(const uint8_t* buffer, size_t size) override;
    virtual size_t write(uint8_t byte) override;
    virtual int availableForWrite() override;
    virtual void flush() override;

private:
    HardwareSerial* _serial;
};
