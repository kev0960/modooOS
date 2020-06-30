#ifndef UTF8_H
#define UTF8_H

// Sets c into string s. Returns the number of bytes written.
template <typename Int>
int SetUnicodeToUTF8(Int c, char* s) {
  if (c <= 0x7F) {
    s[0] = c;
    return 1;
  } else if (c <= 0x7FF) {
    s[0] = 0b11000000 | (c >> 6);
    s[1] = 0b10000000 | c;
    return 2;
  } else if (c <= 0xFFFF) {
    s[0] = 0b11100000 | (c >> 12);
    s[1] = 0b10000000 | (c >> 6);
    s[2] = 0b10000000 | c;
    return 3;
  } else if (c <= 0x10FFFF) {
    s[0] = 0b11110000 | (c >> 18);
    s[1] = 0b10000000 | (c >> 12);
    s[2] = 0b10000000 | (c >> 6);
    s[3] = 0b10000000 | c;
    return 4;
  }
  return 0;
}

#endif
