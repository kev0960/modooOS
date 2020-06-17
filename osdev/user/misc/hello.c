#include <printf.h>
#include <syscall.h>

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

  int pipefd[2];

  // Create a pipe.
  pipe(pipefd);

  pid_t pid;

  // Now child inherits the pipe.
  int result = spawn(&pid, "./print_simple");

  char str_out[128];
  int cnt = read(3, str_out, 128);
  str_out[0] = '!';
  str_out[cnt] = 0;
  printf("%s", str_out);

  cnt = read(3, str_out, 128);
  str_out[0] = '!';
  str_out[cnt] = 0;
  printf("%s", str_out);

  cnt = read(3, str_out, 128);
  str_out[0] = '!';
  str_out[cnt] = 0;
  printf("%s", str_out);

  cnt = read(3, str_out, 128);
  str_out[0] = '!';
  str_out[cnt] = 0;
  printf("%s", str_out);

  cnt = read(3, str_out, 128);
  str_out[0] = '!';
  str_out[cnt] = 0;
  printf("%s", str_out);

  printf("result = %d pid : %d\n", result, pid);
  printf("pipe %d %d \n", pipefd[0], pipefd[1]);

  int status;
  waitpid(pid, &status);
  printf("child exit code : %d\n", status);

  return 0;
}
