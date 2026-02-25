#pragma once
#include <stdint.h>

class UIBuffer {
public:
    static char* getLine(uint8_t lineIndex);
    static const int MAX_LINES = 8;
    static const int LINE_LENGTH = 32;

private:
    static char _lines[MAX_LINES][LINE_LENGTH];
};
