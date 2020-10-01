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

int isdigit(int ch) {
  if ('0' <= ch && ch <= '9') {
    return 1;
  }

  return 0;
}

int isspace(int ch) {
  switch (ch) {
    case ' ':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
      return 1;
  }

  return 0;
}
