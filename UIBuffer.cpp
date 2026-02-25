#include "UIBuffer.h"

char UIBuffer::_lines[UIBuffer::MAX_LINES][UIBuffer::LINE_LENGTH];

char* UIBuffer::getLine(uint8_t lineIndex) {
    if (lineIndex >= MAX_LINES) {
        return _lines[0]; // Fallback to first line to prevent crash
    }
    return _lines[lineIndex];
}
