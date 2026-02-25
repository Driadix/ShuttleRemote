#pragma once
#include "Widget.h"

// Replaces repetitive menu drawing and cursor logic.
// Manages cursor, pagination, and highlight box drawing.
class ScrollingListWidget : public Widget {
public:
    // Callback to populate the buffer for a given item index
    typedef void (*ItemTextProvider)(uint8_t index, char* buffer);

    // provider: Function to get item text
    // itemCount: Total number of items in the list
    // visibleItems: How many lines fit on the screen (e.g., 4)
    ScrollingListWidget(ItemTextProvider provider, uint8_t itemCount, uint8_t visibleItems);

    virtual void draw(U8G2& display, uint8_t x, uint8_t y) override;
    virtual bool handleInput(InputEvent event) override;

    uint8_t getCursorIndex() const { return _cursorPos; }
    void setCursorIndex(uint8_t idx);

private:
    ItemTextProvider _provider;
    uint8_t _itemCount;
    uint8_t _visibleItems;
    uint8_t _cursorPos;
    uint8_t _topIndex; // Index of the first item currently visible
};
