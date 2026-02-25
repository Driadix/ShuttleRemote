#pragma once
#include "Widget.h"

// Replaces repetitive menu drawing and cursor logic.
// Manages cursor, pagination, and highlight box drawing.
class ScrollingListWidget : public Widget {
public:
    // items: Array of string pointers (const char*)
    // itemCount: Total number of items in the list
    // visibleItems: How many lines fit on the screen (e.g., 4)
    ScrollingListWidget(const char* const* items, uint8_t itemCount, uint8_t visibleItems);

    virtual void draw(U8G2& display, uint8_t x, uint8_t y) override;
    virtual bool handleInput(InputEvent event) override;

    uint8_t getCursorIndex() const { return _cursorPos; }
    void setCursorIndex(uint8_t idx);

private:
    const char* const* _items;
    uint8_t _itemCount;
    uint8_t _visibleItems;
    uint8_t _cursorPos;
    uint8_t _topIndex; // Index of the first item currently visible
};
