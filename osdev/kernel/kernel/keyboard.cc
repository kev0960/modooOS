#include "keyboard.h"
#include "char_util.h"
#include "timer.h"
#include "vga_output.h"

namespace Kernel {
namespace {
static const int kScanCodeToASCII[] = {
    0,
    KEY_CODES::ESC,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    KEY_CODES::BACKSPACE,
    KEY_CODES::TAB, /* 0x0F */
    'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '[',
    ']',
    KEY_CODES::ENTER,
    KEY_CODES::LCTRL,
    'a',
    's',
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '`',
    KEY_CODES::LSHIFT,
    '\\',
    'z',
    'x',
    'c',
    'v',
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    KEY_CODES::RSHIFT,
    KEY_CODES::KEYPAD_STAR,
    KEY_CODES::LALT,
    ' ',
    KEY_CODES::CAPSLOCK,
    KEY_CODES::F1,
    KEY_CODES::F2,
    KEY_CODES::F3,
    KEY_CODES::F4,
    KEY_CODES::F5,
    KEY_CODES::F6,
    KEY_CODES::F7,
    KEY_CODES::F8,
    KEY_CODES::F9,
    KEY_CODES::F10,
    KEY_CODES::NUMBER_LOCK,
    KEY_CODES::SCROLL_LOCK,
    KEY_CODES::KEYPAD_7,
    KEY_CODES::KEYPAD_8,
    KEY_CODES::KEYPAD_9,
    KEY_CODES::KEYPAD_MINUS,
    KEY_CODES::KEYPAD_4,
    KEY_CODES::KEYPAD_5,
    KEY_CODES::KEYPAD_6,
    KEY_CODES::KEYPAD_PLUS,
    KEY_CODES::KEYPAD_1,
    KEY_CODES::KEYPAD_2,
    KEY_CODES::KEYPAD_3,
    KEY_CODES::KEYPAD_0,
    KEY_CODES::KEYPAD_DOT,
    0,
    0,
    0,
    KEY_CODES::F11,
    KEY_CODES::F12, /* 0x58 */
};

KeyInfo ScanCodeToASCII(uint8_t scan_code) {
  if (scan_code <= 0x58) {
    return {static_cast<KEY_CODES>(kScanCodeToASCII[scan_code]), KEY_DOWN};
  } else if (0x81 <= scan_code && scan_code <= 0xD8) {
    return {static_cast<KEY_CODES>(kScanCodeToASCII[scan_code - 0x80]), KEY_UP};
  }
  return {CODE_NOT_EXIST, KEY_ERROR};
}

char CharWhenShiftPressed(char c) {
  // 0 .. 9
  char shift_number[] = {')', '!', '@', '#', '$', '%', '^', '&', '*', '('};
  if ('a' <= c && c <= 'z') {
    return ToUpperCase(c);
  } else if ('0' <= c && c <= '9') {
    return shift_number[c - '0'];
  }
  switch (c) {
    case '`':
      return '~';
    case '-':
      return '_';
    case '=':
      return '+';
    case '[':
      return '{';
    case ']':
      return '}';
    case ';':
      return ':';
    case '\'':
      return '"';
    case ',':
      return '<';
    case '.':
      return '>';
    case '/':
      return '?';
  }
  return c;
}

}  // namespace

PS2Keyboard ps2_keyboard;

void PS2Keyboard::MainKeyboardHandler(uint8_t scan_code) {
  KeyInfo key_info = ScanCodeToASCII(scan_code);
  auto& key_press_info = key_press_list_[key_info.key];

  auto& timer = TimerManager::GetTimerManager().GetTimer();

  if (key_info.action == KEY_DOWN) {
    if (key_press_info.time_down == 0) {
      key_press_info.time_down = timer.GetClock();
    } else {
      uint64_t first_key_pressed_time = key_press_info.time_down;
      if (timer.GetClock() - first_key_pressed_time < 40) {
        return;
      }
    }
  } else {
    key_press_info.time_down = 0;
  }

  if (key_info.action == KEY_DOWN) {
    if (key_info.key < 128) {
      if (IsShiftDown()) {
        vga_output << CharWhenShiftPressed(static_cast<char>(key_info.key));
      } else {
        vga_output << static_cast<char>(key_info.key);
      }
    } else {
      if (key_info.key == ENTER) {
        vga_output << "\n";
      }
      // vga_output << static_cast<int>(key_info.key);
    }
  }
}

bool PS2Keyboard::IsShiftDown() const {
  return (key_press_list_[KEY_CODES::LSHIFT].time_down != 0) ||
         (key_press_list_[KEY_CODES::RSHIFT].time_down != 0);
}

bool PS2Keyboard::IsControlDown() const {
  return (key_press_list_[KEY_CODES::LCTRL].time_down != 0) ||
         (key_press_list_[KEY_CODES::RCTRL].time_down != 0);
}

bool PS2Keyboard::IsAltDown() const {
  return key_press_list_[KEY_CODES::LALT].time_down != 0;
}

}  // namespace Kernel
