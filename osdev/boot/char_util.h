#ifndef CHAR_UTIL_H
#define CHAR_UTIL_H

char ToUpperCase(char c) {
  if ('a' <= c && c <= 'z') {
    return c + 'A' - 'a';
  }
  return c;
}

char ToLowerCase(char c) {
  if ('A' <= c && c <= 'Z') {
    return c - ('A' - 'a');
  }
  return c;
}


#endif
