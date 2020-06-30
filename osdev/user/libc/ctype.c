#include "ctype.h"

int tolower(int ch) {
  if ('A' <= ch && ch <= 'Z') {
    return 'a' + (ch - 'A');
  }
  return ch;
}

int toupper(int ch) {
  if ('a' <= ch && ch <= 'z') {
    return 'A' + (ch - 'a');
  }

  return ch;
}
