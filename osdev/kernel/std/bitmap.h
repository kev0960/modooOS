#include "printf.h"
#include "types.h"

namespace Kernel {

constexpr static size_t kBitsPerElem = 8 * sizeof(uint64_t);

template <size_t BitmapSize>
class Bitmap {
 public:
  uint64_t* GetBitmap() { return bitmap; }
  size_t GetBitmapSize() const { return BitmapSize; }

  int GetEmptyBitIndex() {
    for (size_t i = 0; i < BitmapSize / kBitsPerElem; i++) {
      // bitmap :  00000101001
      // inverse : 11111010110
      // builtin_ctz --> Number of trailing zeros.
      uint64_t b = ~bitmap[i];

      // Every bits are occupied.
      if (b == 0) {
        continue;
      }
      int index = __builtin_ctzl(b);
      return i * kBitsPerElem + index;
    }

    return -1;
  }

  void FlipBit(size_t index) {
    uint64_t mask = (1LL << (index % kBitsPerElem));
    bitmap[index / kBitsPerElem] ^= mask;
  }

  bool IsSet(size_t index) {
    uint64_t mask = (1LL << (index % kBitsPerElem));
    return bitmap[index / kBitsPerElem] & mask;
  }

  Bitmap() {
    // Size of the bitmap must be multiple of 64.
    static_assert(BitmapSize % kBitsPerElem == 0);
  }

  Bitmap(const Bitmap& b) {
    for (size_t i = 0; i < BitmapSize / kBitsPerElem; i++) {
      bitmap[i] = b.bitmap[i];
    }
  }

 private:
  uint64_t bitmap[BitmapSize / kBitsPerElem];
};

}  // namespace Kernel
