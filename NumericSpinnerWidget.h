#pragma once
#include "Widget.h"
#include <Arduino.h>

// Multi-digit data entry widget (e.g., PIN codes, Shuttle IDs, Quantities)
class NumericSpinnerWidget : public Widget {
public:
    // numDigits: Number of digits (e.g., 4 for PIN)
    // initialValue: Starting value (e.g., 1441). Negative values will be treated as absolute.
    NumericSpinnerWidget(uint8_t numDigits, int32_t initialValue);

    virtual void draw(U8G2& display, uint8_t x, uint8_t y) override;
    virtual bool handleInput(InputEvent event) override;

    uint32_t getValue() const;

private:
    uint8_t _numDigits;
    int8_t _digits[6]; // Max 6 digits supported (enough for 999999)
    uint8_t _curDigitIdx; // Index of digit currently being edited
};
