/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2023 Uniot <contact@uniot.io>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <Arduino.h>

namespace uniot {

/**
 * @brief Memory manager for a global static buffer with dynamic allocation capabilities
 * @defgroup utils_memory_manager Memory Manager
 * @ingroup utils
 * @{
 *
 * This class implements a simple memory manager that operates on a static buffer.
 * It provides dynamic memory allocation within the buffer, using a free-list approach
 * to track available memory blocks. The memory manager supports allocation, deallocation,
 * and reallocation operations, making it useful for environments with limited heap
 * availability or when more controlled memory management is required.
 */
class GlobalBufferMemoryManager {
 public:
  /**
   * @brief Initialize the memory manager
   *
   * Sets up the free list with a single large block covering the entire buffer.
   * Must be called before any other memory operations.
   */
  static void initialize();

  /**
   * @brief Allocate memory from the global buffer
   *
   * @param size Number of bytes to allocate
   * @retval void* Pointer to the allocated memory block
   * @retval nullptr If allocation fails
   */
  static void* allocate(size_t size);

  /**
   * @brief Deallocate previously allocated memory
   *
   * Returns the memory block to the free list, merging with adjacent free blocks when possible.
   *
   * @param ptr Pointer to memory previously allocated with allocate()
   */
  static void deallocate(void* ptr);

  /**
   * @brief Resize allocated memory
   *
   * Either returns the existing block if it's large enough, or allocates a new block,
   * copies the data, and frees the old block.
   *
   * @param ptr Pointer to memory previously allocated with allocate(), or nullptr
   * @param newSize New size in bytes
   * @retval void* Pointer to the resized memory block
   * @retval nullptr If reallocation fails
   */
  static void* reallocate(void* ptr, size_t newSize);

  /**
   * @brief Get the total free memory available in the buffer
   *
   * @retval size_t Total bytes available across all free blocks
   */
  static size_t getTotalFreeMemory();

  /**
   * @brief Get the size of the largest available free block
   *
   * @retval size_t Size in bytes of the largest contiguous free block
   */
  static size_t getLargestFreeBlock();

 private:
  /**
   * @brief Structure representing a free memory block in the free list
   *
   * Each free block tracks its size and contains a pointer to the next free block.
   * This creates a linked list of available memory regions.
   */
  struct FreeBlock {
    size_t size;     ///< Size of this free block in bytes
    FreeBlock* next; ///< Pointer to the next free block in the list
  };

  /// Flag to enable/disable debug output
  static constexpr bool DEBUG = false;

  /// Size of the global buffer in bytes
  static constexpr size_t BUFFER_SIZE = 4096;

  /// The global buffer storage area
  static uint8_t mGlobalBuffer[BUFFER_SIZE];

  /// Head of the free block list
  static FreeBlock* mFreeList;
};
/** @} */
}  // namespace uniot
