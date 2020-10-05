#include <ctype.h>
#include <scanf.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * ./m_config.c:        sscanf(strparm+2, "%x", &parm);
./m_config.c:        sscanf(strparm, "%i", &parm);
./m_config.c:        if (fscanf(f, "%79s %99[^\n]\n", defname, strparm) != 2)
./m_misc.c:  return sscanf(str, " 0x%x", result) == 1 ||
./m_misc.c:         sscanf(str, " 0X%x", result) == 1 ||
./m_misc.c:         sscanf(str, " 0%o", result) == 1 || sscanf(str, " %d",
result) == 1;
./doomgeneric_soso.c:        sscanf(myargv[argPosX + 1], "%d", &s_PositionX);
./doomgeneric_soso.c:        sscanf(myargv[argPosY + 1], "%d", &s_PositionY);
*/

int GetHexValue(char c) {
  if (isdigit(c)) {
    return c - '0';
  }

  if ('a' <= tolower(c) && tolower(c) <= 'f') {
    return tolower(c) - 'a' + 10;
  }

  return -1;
}

int MatchDecimal(const char* buffer, int current, int* num) {
  while (isspace(buffer[current])) {
    current++;
  }

  *num = 0;
  int sign = 1;

  if (buffer[current] == '-') {
    sign = -1;
    current++;
  } else if (buffer[current] == '+') {
    current++;
  }

  for (; buffer[current] != 0; current++) {
    if (!isdigit(buffer[current])) {
      *num = (*num) * sign;
      return current;
    }
    *num = (*num) * 10 + (buffer[current] - '0');
  }

  *num = (*num) * sign;
  return current;
}

int MatchHex(const char* buffer, int current, int* num) {
  while (isspace(buffer[current])) {
    current++;
  }

  *num = 0;
  int sign = 1;

  if (buffer[current] == '-') {
    sign = -1;
    current++;
  } else if (buffer[current] == '+') {
    current++;
  }

  // Ignore prefix "0x"
  if (buffer[current] == '0' &&
      (buffer[current + 1] == 'x' || buffer[current + 1] == 'X')) {
    current += 2;
  }

  for (; buffer[current] != 0; current++) {
    int digit = GetHexValue(buffer[current]);
    if (digit == -1) {
      *num = (*num) * sign;
      return current;
    }
    *num = (*num) * 16 + digit;
  }

  *num = (*num) * sign;
  return current;
}

int MatchOct(const char* buffer, int current, int* num) {
  while (isspace(buffer[current])) {
    current++;
  }

  *num = 0;
  int sign = 1;

  if (buffer[current] == '-') {
    sign = -1;
    current++;
  } else if (buffer[current] == '+') {
    current++;
  }

  // Ignore prefix "0"
  if (buffer[current] == '0') {
    current += 1;
  }

  for (; buffer[current] != 0; current++) {
    if (!('0' <= buffer[current] && buffer[current] <= '7')) {
      *num = (*num) * sign;
      return current;
    }
    *num = (*num) * 8 + (buffer[current] - '0');
  }

  *num = (*num) * sign;
  return current;
}

int sscanf(const char* buffer, const char* format, ...) {
  int buffer_pos = 0;

  size_t format_length = strlen(format);
  bool introduced_percent = false;

  va_list args;
  va_start(args, format);

  for (size_t i = 0; i < format_length; i++) {
    if (isspace(format[i])) {
      // Consume every consecutive spaces in the buffer.
      while (isspace(buffer[buffer_pos])) {
        buffer_pos++;
      }
    } else if (format[i] == '%') {
      if (!introduced_percent) {
        introduced_percent = true;
      } else {
        if (buffer[buffer_pos] != '%') {
          return buffer_pos;
        } else {
          introduced_percent = false;
          buffer_pos++;
        }
      }
    } else {
      if (introduced_percent) {
        switch (format[i]) {
          case 'd':
            // Match decimals
            buffer_pos = MatchDecimal(buffer, buffer_pos, va_arg(args, int*));
            break;
          case 'x':
            // Match Hex
            buffer_pos = MatchHex(buffer, buffer_pos, va_arg(args, int*));
            break;
          case 'o':
            // Match Oct
            buffer_pos = MatchOct(buffer, buffer_pos, va_arg(args, int*));
            break;
        }

        introduced_percent = false;
      } else {
        if (format[i] == buffer[buffer_pos]) {
          buffer_pos++;
        } else {
          return buffer_pos;
        }
      }
    }
  }

  return buffer_pos;
}
