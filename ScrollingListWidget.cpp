#include "ScrollingListWidget.h"
#include <Arduino.h>

ScrollingListWidget::ScrollingListWidget(ItemTextProvider provider, uint8_t itemCount, uint8_t visibleItems)
    : Widget(128, visibleItems * 11), // 11px per item natively fits 5 on screen 
      _provider(provider), _itemCount(itemCount), _visibleItems(visibleItems),
      _cursorPos(0), _topIndex(0) {
}

void ScrollingListWidget::setCursorIndex(uint8_t idx) {
    if (idx >= _itemCount) idx = _itemCount - 1;
    _cursorPos = idx;
    if (_cursorPos < _topIndex) {
        _topIndex = _cursorPos;
    } else if (_cursorPos >= _topIndex + _visibleItems) {
        _topIndex = _cursorPos - _visibleItems + 1;
    }
}

bool ScrollingListWidget::handleInput(InputEvent event) {
    bool changed = false;

    if (event == InputEvent::UP_PRESS) {
        if (_cursorPos > 0) {
            _cursorPos--;
            if (_cursorPos < _topIndex) _topIndex--;
        } else {
            _cursorPos = _itemCount - 1;
            if (_itemCount > _visibleItems) _topIndex = _itemCount - _visibleItems;
            else _topIndex = 0;
        }
        changed = true;
    } else if (event == InputEvent::DOWN_PRESS) {
        if (_cursorPos < _itemCount - 1) {
            _cursorPos++;
            if (_cursorPos >= _topIndex + _visibleItems) _topIndex++;
        } else {
            _cursorPos = 0;
            _topIndex = 0;
        }
        changed = true;
    }

    if (changed) setDirty();
    return changed;
}

void ScrollingListWidget::draw(U8G2& display, uint8_t x, uint8_t y) {
    display.setFont(u8g2_font_6x13_t_cyrillic);
    const uint8_t lineHeight = 12;

    char buffer[64];

    for (uint8_t i = 0; i < _visibleItems; i++) {
        uint8_t realIndex = _topIndex + i;
        if (realIndex >= _itemCount) break;

        _provider(realIndex, buffer);

        uint8_t boxY = y + (i * lineHeight);
        uint8_t textY = boxY + 11;

        if (realIndex == _cursorPos) {
            display.setDrawColor(1);
            display.drawBox(x, boxY, 128, lineHeight);
            display.setDrawColor(0); 
            display.setCursor(x + 2, textY);
            display.print(buffer);
            display.setDrawColor(1); 
        } else {
            display.setDrawColor(1); 
            display.setCursor(x + 2, textY);
            display.print(buffer);
        }
    }
}