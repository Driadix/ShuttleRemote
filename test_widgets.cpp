#include <Arduino.h>
#include <U8g2lib.h>
#include "ScrollingListWidget.h"
#include "NumericSpinnerWidget.h"
#include "StatusBarWidget.h"
#include "InputEvents.h"

// Mock U8G2 for testing logic (methods would be empty or logging)
// Since we can't mock fully, we just instantiate widgets and call handleInput.

void test_scrolling_list() {
    Serial.println("--- Test ScrollingListWidget ---");
    const char* items[] = { "Item 1", "Item 2", "Item 3", "Item 4", "Item 5", "Item 6" };
    ScrollingListWidget list(items, 6, 4);

    // Initial State
    Serial.printf("Initial Cursor: %d\n", list.getCursorIndex()); // Expected: 0

    // Scroll Down
    list.handleInput(InputEvent::DOWN_PRESS);
    Serial.printf("After DOWN: %d\n", list.getCursorIndex()); // Expected: 1

    // Scroll Down to bottom
    list.handleInput(InputEvent::DOWN_PRESS); // 2
    list.handleInput(InputEvent::DOWN_PRESS); // 3
    list.handleInput(InputEvent::DOWN_PRESS); // 4 (Scrolls View)
    list.handleInput(InputEvent::DOWN_PRESS); // 5 (End)
    Serial.printf("At Bottom: %d\n", list.getCursorIndex()); // Expected: 5

    // Scroll Down (Wrap)
    list.handleInput(InputEvent::DOWN_PRESS);
    Serial.printf("After Wrap: %d\n", list.getCursorIndex()); // Expected: 0

    // Scroll Up (Wrap)
    list.handleInput(InputEvent::UP_PRESS);
    Serial.printf("After Up Wrap: %d\n", list.getCursorIndex()); // Expected: 5
}

void test_numeric_spinner() {
    Serial.println("--- Test NumericSpinnerWidget ---");
    NumericSpinnerWidget spinner(4, 1234);

    // Initial State
    Serial.printf("Initial Value: %d\n", spinner.getValue()); // Expected: 1234

    // Edit MSD (Index 0: '1') -> Up -> '2'
    spinner.handleInput(InputEvent::UP_PRESS);
    Serial.printf("After UP on digit 0: %d\n", spinner.getValue()); // Expected: 2234

    // Move to next digit (Index 1: '2')
    spinner.handleInput(InputEvent::OK_SHORT_PRESS);

    // Edit Index 1 -> Down -> '1'
    spinner.handleInput(InputEvent::DOWN_PRESS);
    Serial.printf("After DOWN on digit 1: %d\n", spinner.getValue()); // Expected: 2134
}

void test_status_bar() {
    Serial.println("--- Test StatusBarWidget ---");
    StatusBarWidget statusBar;
    // Cannot easily test draw without display, but instantiation verifies compilation structure.
    Serial.println("StatusBarWidget instantiated.");
}

void setup() {
    Serial.begin(115200);
    test_scrolling_list();
    test_numeric_spinner();
    test_status_bar();
}

void loop() {
}
