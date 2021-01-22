#include "stack_allocator.h"

uint8_t *StackAllocator::m_base {};
uint8_t *StackAllocator::m_head {};
uint8_t *StackAllocator::m_end  {};
