#pragma once

#include <cstdint>
#include <cstdlib>
#include <cassert>

class StackAllocator {
  static uint8_t *m_base;
  static uint8_t *m_head;
  static uint8_t *m_end;

  StackAllocator() = delete;

  static constexpr size_t alignment = 64;

public:
  static void init(size_t size) {
    m_base = static_cast<uint8_t *>(aligned_alloc(alignment, size));
    m_head = m_base;
    m_end  = m_base + size;
  }

  static void cleanup() {
    ::free(m_base);
  }

  static void *allocate(size_t size) {
    size_t bytes = (size + alignment - 1) & -alignment;
    assert(m_head + bytes < m_end);
    void *ptr = m_head;
    m_head += bytes;
    return ptr;
  }

  static void free(void *ptr) {
    m_head = static_cast<uint8_t *>(ptr);
  }
};
