#include <printf.h>
#include <stdlib.h>
#include <syscall.h>

int main() {
  struct ScreenInfo info;
  screen(GET_SCREEN_INFO, &info, NULL);

  printf("Screen height : %d width %d Pixel size :%d \n", info.height,
         info.width, info.pixel_size);

  uint32_t* buffer = malloc(5 * 10 * info.pixel_size);
  for (int i = 0; i < 50; i++) {
    buffer[i] = 0xff00ff;
  }

  struct FrameBufferInfo binfo;
  binfo.screen_row = 123;
  binfo.screen_col = 123;
  binfo.buffer_width = 5;
  binfo.buffer_height = 10;

  screen(COPY_FRAME_BUFFER, buffer, &binfo);
  return 0;
}
