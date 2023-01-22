#pragma once

#include <cstdint>

#include "FreeRTOS.h"

void* operator new(std::size_t size);
void operator delete(void* ptr) noexcept;
namespace System {
class Memory {
 public:
  static void* Malloc(size_t size);
  static void Free(void* block);
};
}  // namespace System
