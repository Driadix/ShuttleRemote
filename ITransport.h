#pragma once
#include <stdint.h>
#include <stddef.h>

// Abstract transport layer interface
class ITransport {
public:
    virtual ~ITransport() {}

    // Check if data is available to read
    virtual int available() = 0;

    // Read a byte
    virtual int read() = 0;

    // Write data
    virtual size_t write(const uint8_t* buffer, size_t size) = 0;
    virtual size_t write(uint8_t byte) = 0;

    // Check available write buffer space
    virtual int availableForWrite() = 0;

    // Flush TX buffer
    virtual void flush() = 0;
};
