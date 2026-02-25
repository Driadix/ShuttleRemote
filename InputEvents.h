#ifndef INPUT_EVENTS_H
#define INPUT_EVENTS_H

enum class InputEvent {
  NONE,
  UP_PRESS,            // '8'
  DOWN_PRESS,          // '0'
  OK_SHORT_PRESS,      // 'D' (Short)
  OK_LONG_PRESS,       // 'D' (Long)
  BACK_PRESS,          // '7'
  STOP_PRESS,          // '6' (Short)
  MANUAL_MODE_PRESS,   // '6' (Long)
  LOAD_PRESS,          // '5' (Short)
  LONG_LOAD_PRESS,     // '5' (Long)
  UNLOAD_PRESS,        // 'C' (Short)
  LONG_UNLOAD_PRESS,   // 'C' (Long)
  LIFT_UP_PRESS,       // 'E'
  LIFT_DOWN_PRESS,     // '9'
  DEMO_PRESS,          // (Reserved/Logical)
  // Direct key mapping if needed for specific shuttle selection
  KEY_1_PRESS,
  KEY_2_PRESS,
  KEY_3_PRESS,
  KEY_4_PRESS,
  KEY_A_PRESS,
  KEY_B_PRESS
};

#endif // INPUT_EVENTS_H
