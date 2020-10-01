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
    printf("buf : %c\n", buffer[current]);
    if (!isdigit(buffer[current])) {
      *num = (*num) * sign;
      return current;
    }
    *num = (*num) * 10 + (buffer[current] - '0');
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
    printf("format: %c %d\n", format[i], introduced_percent);
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
            printf("Match d : %c\n", format[i]);
            // Match decimals
            buffer_pos = MatchDecimal(buffer, buffer_pos, va_arg(args, int*));
            break;
          case 'x':
            // Match Hex
            buffer_pos = MatchDecimal(buffer, buffer_pos, va_arg(args, int*));
            break;
          case 'o':
            // Match Oct
            buffer_pos = MatchDecimal(buffer, buffer_pos, va_arg(args, int*));
            break;
        }
      }
    }
  }

  return buffer_pos;
}
