#ifndef SYS_SYS_H
#define SYS_SYS_H

namespace Kernel {

template <typename HandlerImpl>
class SyscallHandler {
 public:
   static HandlerImpl& GetHandler() {
     static HandlerImpl handler;
     return handler;
   }

 protected:
   SyscallHandler() = default;

};
}  // namespace Kernel

#endif
