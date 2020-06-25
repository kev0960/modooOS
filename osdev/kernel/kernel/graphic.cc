#include "graphic.h"

#include "../boot/multiboot2.h"
#include "kmalloc.h"
#include "paging.h"
#include "qemu_log.h"

namespace Kernel {
namespace {

uint64_t RoundUpToFourKB(uint64_t sz) {
  constexpr uint64_t kFourKB = (1 << 12);
  if (sz % kFourKB == 0) {
    return sz;
  }
  return (sz / kFourKB + 1) * kFourKB;
}

}  // namespace

void ParseMultibootInfo(void* multiboot_info) {
  uint8_t* info_addr = reinterpret_cast<uint8_t*>(
      PageTableManager::kKernelVMStart + (uint64_t)(multiboot_info));
  QemuSerialLog::Logf("info addr : %lx\n", info_addr);
  QemuSerialLog::Logf("reserved : %lx\n", *(uint32_t*)(info_addr + 4));

  uint32_t total_size = *(uint32_t*)info_addr;
  QemuSerialLog::Logf("total_size %lx\n", *(uint32_t*)info_addr);

  uint32_t index = 8;
  while (index < total_size) {
    uint32_t tag_type = *(uint32_t*)(info_addr + index);
    if (tag_type == MULTIBOOT_TAG_TYPE_END) {
      break;
    }
    if (tag_type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
      multiboot_tag_framebuffer_common* common =
          (multiboot_tag_framebuffer_common*)(info_addr + index);
      GraphicManager::GetGraphicManager().Init(
          common->framebuffer_width, common->framebuffer_height,
          common->framebuffer_bpp, (uint32_t*)common->framebuffer_addr);
    }

    uint32_t tag_size = *(uint32_t*)(info_addr + index + 4);
    // QemuSerialLog::Logf("Type : %d \n", tag_type);
    // QemuSerialLog::Logf("size : %d \n", tag_size);
    index += tag_size;
    if (index % 8 != 0) {
      index += (8 - index % 8);
    }
  }
}

void GraphicManager::Init(int width, int height, int pixel_size,
                          uint32_t* video_mem_phys) {
  QemuSerialLog::Logf("width : %d height %d pixel_size : %d vmem : %lx", width,
                      height, pixel_size, video_mem_phys);
  width_ = width;
  height_ = height;
  pixel_size_ = pixel_size;

  uint64_t video_mem_size = RoundUpToFourKB(width * height * (pixel_size_ / 8));

  // Now allocate the memory.
  video_mem_ = (uint32_t*)kaligned_alloc((1 << 12), video_mem_size);

  // Map page table.
  PageTableManager::GetPageTableManager().AllocateKernelPage(
      (uint64_t)video_mem_, video_mem_size, (uint64_t)video_mem_phys);
}

void GraphicManager::DrawAt(int row, int col, Color color) {
  if (video_mem_ == nullptr) {
    return;
  }

  video_mem_[width_ * row + col] = color;
}

}  // namespace Kernel
