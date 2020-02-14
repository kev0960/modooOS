namespace Kernel {

class SyscallManager {
 public:
  static SyscallManager& GetSyscallManager() {
    static SyscallManager syscall_manager;
    return syscall_manager;
  }

  static void SyscallHandler();

 private:

  SyscallManager();
};

}  // namespace Kernel
