#ifndef INPUT_EVENTS_H
#define INPUT_EVENTS_H

enum class InputEvent {
  NONE,
  UP_PRESS,            // '8' - Navigate Up
  DOWN_PRESS,          // '0' - Navigate Down
  LEFT_PRESS,          // '9' - Navigate Left / Decrease Value
  RIGHT_PRESS,         // 'E' - Navigate Right / Increase Value
  OK_SHORT_PRESS,      // 'D' (Short) - Confirm/Enter
  OK_LONG_PRESS,       // 'D' (Long)
  BACK_PRESS,          // '7' - Go Back
  STOP_PRESS,          // '6' (Short)
  MANUAL_MODE_PRESS,   // '6' (Long)
  LOAD_PRESS,          // '5' (Short)
  LONG_LOAD_PRESS,     // '5' (Long)
  UNLOAD_PRESS,        // 'C' (Short)
  LONG_UNLOAD_PRESS,   // 'C' (Long)
  LIFT_UP_PRESS,       // 'E' - Lift Pallet (Hardware direct, aliases Right)
  LIFT_DOWN_PRESS,     // '9' - Drop Pallet (Hardware direct, aliases Left)
  DEMO_PRESS,          // (Reserved/Logical)
  // Direct key mapping for specific shuttle selection on dashboard
  KEY_1_PRESS,
  KEY_2_PRESS,
  KEY_3_PRESS,
  KEY_4_PRESS,
  KEY_A_PRESS,
  KEY_B_PRESS
};

#endif // INPUT_EVENTS_H