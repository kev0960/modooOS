#include "kernel_math.h"

namespace Kernel {

int RoundUpNearestPowerOfTwoLog(uint32_t bytes) {
  // For example,
  // 0000 0000 | 0000 0000 | 0000 0001 | 0010 0000
  // num_leading_zeros : 23
  // num_trailing_zeros : 5
  // --> returns 32 - 23 = 9 (2^9)
  //
  // 0000 0000 | 0000 0000 | 0000 0001 | 0000 0000
  // num_leading_zeros : 23
  // num_trailing_zeros : 8 (+ 1) = 9
  // --> returns 9 - 1 = 8 (2^8).
  int num_leading_zeros = __builtin_clz(bytes);
  int num_trailing_zeros = __builtin_ffs(bytes);

  // bytes is the power of 2.
  if (num_leading_zeros + num_trailing_zeros == 32) {
    return num_trailing_zeros - 1;
  } else {
    return 32 - num_leading_zeros;
  }
}

int RoundUpNearestPowerOfTwoLog(uint64_t bytes) {
  int num_leading_zeros = __builtin_clzll(bytes);
  int num_trailing_zeros = __builtin_ffsll(bytes);

  // bytes is the power of 2.
  if (num_leading_zeros + num_trailing_zeros == 64) {
    return num_trailing_zeros - 1;
  } else {
    return 64 - num_leading_zeros;
  }
}

}  // namespace Kernel
