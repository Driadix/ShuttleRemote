#include "UartTransport.h"

UartTransport::UartTransport(HardwareSerial* serial) : _serial(serial) {
}

int UartTransport::available() {
    return _serial ? _serial->available() : 0;
}

int UartTransport::read() {
    return _serial ? _serial->read() : -1;
}

size_t UartTransport::write(const uint8_t* buffer, size_t size) {
    return _serial ? _serial->write(buffer, size) : 0;
}

size_t UartTransport::write(uint8_t byte) {
    return _serial ? _serial->write(byte) : 0;
}

int UartTransport::availableForWrite() {
    return _serial ? _serial->availableForWrite() : 0;
}

void UartTransport::flush() {
    if (_serial) _serial->flush();
}
