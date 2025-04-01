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

#include "GlobalBufferMemoryManager.h"

#include <Logger.h>

#include <cassert>

namespace uniot {

// Initialize static members
uint8_t GlobalBufferMemoryManager::mGlobalBuffer[GlobalBufferMemoryManager::BUFFER_SIZE] = {0};
GlobalBufferMemoryManager::FreeBlock* GlobalBufferMemoryManager::mFreeList = nullptr;

/**
 * @brief Align a size value to a 4-byte boundary
 * @ingroup utils_memory_manager
 *
 * Ensures memory allocations maintain proper alignment for most architectures.
 *
 * @param size Size to align
 * @retval size_t Aligned size (rounded up to nearest 4-byte boundary)
 */
size_t alignSize(size_t size) {
  const size_t alignment = 4;  // 4-byte alignment
  return (size + alignment - 1) & ~(alignment - 1);
}

void GlobalBufferMemoryManager::initialize() {
  // Initialize the free list with a single block spanning the entire buffer
  mFreeList = reinterpret_cast<FreeBlock*>(mGlobalBuffer);
  mFreeList->size = BUFFER_SIZE;
  mFreeList->next = nullptr;
}

void* GlobalBufferMemoryManager::allocate(size_t size) {
  FreeBlock** prev = &mFreeList;
  FreeBlock* block = mFreeList;

  // Calculate required size with alignment, adding space for the block header
  size_t alignedSize = alignSize(size + sizeof(FreeBlock)) - sizeof(FreeBlock);

  // Ensure the block size is at least as large as FreeBlock
  // This guarantees we can reuse the memory as a FreeBlock when deallocated
  if (alignedSize < sizeof(FreeBlock)) {
    alignedSize = sizeof(FreeBlock);
  }

  UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: Requested size: %d, Adjusted size after alignment: %d", size, alignedSize);

  // Search through free list for a suitable block
  while (block) {
    UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: Checking block of size %d", block->size);

    // Safety check against corrupted memory
    if (block->size < sizeof(FreeBlock)) {
      UNIOT_LOG_ERROR("GlobalBufferMemoryManager: Detected a block with invalid size: %d. Possible memory corruption!", block->size);
      return nullptr;
    }

    // Check if this block is large enough for our needs
    if (block->size >= alignedSize + sizeof(FreeBlock)) {
      size_t nextBlockSize = block->size - alignedSize - sizeof(FreeBlock);

      // Only split if the remaining part can form a valid free block
      if (nextBlockSize >= sizeof(FreeBlock) && block->size > alignedSize + 2 * sizeof(FreeBlock)) {
        // Split the block into allocated part and new free block
        FreeBlock* nextBlock = reinterpret_cast<FreeBlock*>(reinterpret_cast<uint8_t*>(block) + alignedSize + sizeof(FreeBlock));
        nextBlock->size = nextBlockSize;
        nextBlock->next = block->next;

        block->size = alignedSize;
        block->next = nextBlock;

        // Update the free list to point to the new free block
        *prev = nextBlock;

        UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: Block split done, new block of size %d and next block of size %d", block->size, nextBlock->size);
      } else {
        // If we can't split, remove the entire block from the free list
        *prev = block->next;
      }

      UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: allocated %d bytes", block->size);
      // Return pointer to user memory (after the block header)
      return reinterpret_cast<uint8_t*>(block) + sizeof(FreeBlock);
    }

    // Move to the next free block
    prev = &block->next;
    block = block->next;
  }

  // No suitable block was found
  UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: no suitable block was found for %d bytes", alignedSize);
  return nullptr;
}

void GlobalBufferMemoryManager::deallocate(void* ptr) {
  if (!ptr) return;  // Null pointer check

  // Get the block header from the user pointer
  FreeBlock* blockToFree = reinterpret_cast<FreeBlock*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(FreeBlock));

  FreeBlock* prev = nullptr;
  FreeBlock* current = mFreeList;

  // Find the correct position to insert this block (free list is sorted by address)
  while (current && current < blockToFree) {
    prev = current;
    current = current->next;
  }

  if (prev) {
    // Check if we can merge with the previous block (blocks are adjacent)
    if (reinterpret_cast<uint8_t*>(prev) + prev->size + sizeof(FreeBlock) == reinterpret_cast<uint8_t*>(blockToFree)) {
      // Merge by extending the previous block
      prev->size += blockToFree->size + sizeof(FreeBlock);
      blockToFree = prev;  // For potential merge with next block
    } else {
      // If not adjacent, just link them
      prev->next = blockToFree;
    }
  } else {
    // This will be the new head of the free list
    mFreeList = blockToFree;
  }

  // Check if we can also merge with the next block
  if (current && reinterpret_cast<uint8_t*>(blockToFree) + blockToFree->size + sizeof(FreeBlock) == reinterpret_cast<uint8_t*>(current)) {
    // Merge by extending the current block and linking to current's next
    blockToFree->size += current->size + sizeof(FreeBlock);
    blockToFree->next = current->next;
  } else {
    // If not adjacent, just link them
    blockToFree->next = current;
  }

  UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: deallocated %d bytes", blockToFree->size - sizeof(FreeBlock));
}

void* GlobalBufferMemoryManager::reallocate(void* ptr, size_t newSize) {
  UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: Entering reallocate. Requested newSize: %d", newSize);

  // If original pointer is null, this is equivalent to malloc
  if (!ptr) {
    UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: ptr is null. Redirecting to allocate.");
    return allocate(newSize);
  }

  // Get the original block header
  FreeBlock* block = reinterpret_cast<FreeBlock*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(FreeBlock));

  UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: Current block size: %d, sizeof(FreeBlock): %d", block->size, sizeof(FreeBlock));

  // Safety check against corrupted memory
  if (block->size < 0) {
    UNIOT_LOG_ERROR("GlobalBufferMemoryManager: Detected negative block size: %d. Possible memory corruption!", block->size);
    return nullptr;
  }

  assert(block->size >= sizeof(FreeBlock));

  // If the current block is large enough, just reuse it
  if (block->size >= newSize) {
    UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: Existing block size is sufficient. Exiting reallocate.");
    return ptr;
  }

  // Otherwise, allocate a new block, copy data, and free the old one
  void* newPtr = allocate(newSize);
  if (newPtr) {
    memcpy(newPtr, ptr, block->size);  // Copy only the valid data from old block
    deallocate(ptr);
    UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: Successfully reallocated to new block. Exiting reallocate.");
  } else {
    UNIOT_LOG_ERROR("GlobalBufferMemoryManager: Failed to reallocate. Exiting reallocate.");
  }

  return newPtr;
}

size_t GlobalBufferMemoryManager::getTotalFreeMemory() {
  size_t totalFree = 0;
  FreeBlock* block = mFreeList;

  // Sum up the sizes of all free blocks
  while (block) {
    totalFree += block->size;
    block = block->next;
  }
  return totalFree;
}

size_t GlobalBufferMemoryManager::getLargestFreeBlock() {
  size_t largestBlock = 0;
  FreeBlock* block = mFreeList;

  // Find the largest free block
  while (block) {
    if (block->size > largestBlock) {
      largestBlock = block->size;
    }
    block = block->next;
  }
  return largestBlock;
}

}  // namespace uniot
