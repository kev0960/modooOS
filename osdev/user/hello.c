#include "./libc/printf.h"
#include "./libc/syscall.h"
#include "sha1.h"

int main() {
  /*
  //const char* s = "abc";
  for (int i = 0; i < 10; i++) {
    write("Hello World this is printed from the process!\n");
  }
  */
  /*
  for (;;) {
    for (int i = 0; i < 10; i++) {
      uint8_t output[20];

      SHA1_CTX ctx;
      SHA1Init(&ctx);

      const char* buf = "abc";
      SHA1Update(&ctx, (uint8_t*)buf, 3);
      SHA1Final(output, &ctx);

      char str_out[42];
      for (int i = 0; i < 20; i++) {
        sprintf(&str_out[2 * i], "%02x", output[i]);
      }
      str_out[40] = '\n';
      str_out[41] = '\0';
      write(str_out);
    }
  }
  */

  pid_t pid;
  int result = spawn(&pid, "/print_simple");

  char str_out[100];
  sprintf(str_out, "result = %d pid : %d\n", result, pid);
  write(str_out);

  int status;
  waitpid(pid, &status);
  sprintf(str_out, "child exit code : %d\n", status);
  write(str_out);

  return 0;
}
