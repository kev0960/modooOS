#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

namespace Kernel {

enum KEY_CODES {
  ESC = 256,
  BACKSPACE,
  TAB,
  LSHIFT,
  RSHIFT,
  LCTRL,
  RCTRL,
  CAPSLOCK,
  SCROLL_LOCK,
  ENTER,
  KEYPAD_STAR,
  LALT,
  SPACE,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  NUMBER_LOCK,
  KEYPAD_7,
  KEYPAD_8,
  KEYPAD_9,
  KEYPAD_MINUS,
  KEYPAD_4,
  KEYPAD_5,
  KEYPAD_6,
  KEYPAD_PLUS,
  KEYPAD_1,
  KEYPAD_2,
  KEYPAD_3,
  KEYPAD_0,
  KEYPAD_DOT,
  F11,
  F12,
  CODE_NOT_EXIST
};

enum KEY_ACTION { KEY_DOWN, KEY_UP, KEY_ERROR };

struct KeyInfo {
  KEY_CODES key;
  KEY_ACTION action;
};

class PS2Keyboard {
 public:
  PS2Keyboard() = default;

  void MainKeyboardHandler(uint8_t scan_code);
};

extern PS2Keyboard ps2_keyboard;
}  // namespace Kernel
#endif
