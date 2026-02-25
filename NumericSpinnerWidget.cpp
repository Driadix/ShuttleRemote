#include "NumericSpinnerWidget.h"
#include <stdlib.h> // for abs

NumericSpinnerWidget::NumericSpinnerWidget(uint8_t numDigits, int32_t initialValue)
    : Widget(numDigits * 12, 16), _numDigits(numDigits), _curDigitIdx(0) {

    if (_numDigits > 6) _numDigits = 6;
    // Recalculate width just in case numDigits was clamped
    setSize(_numDigits * 12, 16);

    int32_t temp = abs(initialValue);
    // Fill digits from right to left
    for (int8_t i = _numDigits - 1; i >= 0; i--) {
        _digits[i] = temp % 10;
        temp /= 10;
    }
}

uint32_t NumericSpinnerWidget::getValue() const {
    uint32_t val = 0;
    for (uint8_t i = 0; i < _numDigits; i++) {
        val = val * 10 + _digits[i];
    }
    return val;
}

bool NumericSpinnerWidget::handleInput(InputEvent event) {
    bool changed = false;

    // Use current digit index from state

    // Logic: Up/Down changes current digit. OK moves to next digit.
    // What if OK is pressed on last digit? Loop back or return true (finished)?
    // The current logic loops back.

    if (event == InputEvent::UP_PRESS) {
        _digits[_curDigitIdx]++;
        if (_digits[_curDigitIdx] > 9) _digits[_curDigitIdx] = 0;
        changed = true;
    } else if (event == InputEvent::DOWN_PRESS) {
        _digits[_curDigitIdx]--;
        if (_digits[_curDigitIdx] < 0) _digits[_curDigitIdx] = 9;
        changed = true;
    } else if (event == InputEvent::OK_SHORT_PRESS) {
        _curDigitIdx++;
        if (_curDigitIdx >= _numDigits) _curDigitIdx = 0;
        changed = true;
    }

    if (changed) setDirty();
    return changed;
}

void NumericSpinnerWidget::draw(U8G2& display, uint8_t x, uint8_t y) {
    display.setFont(u8g2_font_9x15_t_cyrillic); // Larger font for numbers
    // 9x15 font. Baseline approx 12px from top.

    // Draw digits
    for (uint8_t i = 0; i < _numDigits; i++) {
        uint8_t digitX = x + (i * 12); // Spacing

        // Draw underline if active
        if (i == _curDigitIdx) {
            display.drawBox(digitX, y + 14, 9, 2); // Underline at bottom (y+14 to y+16)
        }

        display.setCursor(digitX, y + 12); // Baseline
        display.print(_digits[i]);
    }
}
