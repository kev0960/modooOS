#ifndef SYS_SYS_SCREEN_H
#define SYS_SYS_SCREEN_H

#include "../console.h"
#include "../graphic.h"
#include "../process.h"
#include "sys.h"

namespace Kernel {

enum SysScreenCommands { GET_SCREEN_INFO, COPY_FRAME_BUFFER };

struct ScreenInfo {
  int width;
  int height;
  int pixel_size;
};

class SysScreenHandler : public SyscallHandler<SysScreenHandler> {
 public:
  size_t SysScreen(SysScreenCommands command, void* arg1, void* arg2) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());

    auto& gm = GraphicManager::GetGraphicManager();
    if (command == GET_SCREEN_INFO) {
      ScreenInfo* screen_info = reinterpret_cast<ScreenInfo*>(arg1);
      screen_info->width = gm.GetWidth();
      screen_info->height = gm.GetHeight();
      screen_info->pixel_size = gm.GetPixelSize();

      return 0;
    } else if (command == COPY_FRAME_BUFFER) {
      gm.SyncScreenWith(
          reinterpret_cast<uint32_t*>(arg1),
          reinterpret_cast<GraphicManager::FrameBufferInfo*>(arg2));
      return 0;
    }
    return 1;
  }
};

}  // namespace Kernel

#endif
