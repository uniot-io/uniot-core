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

uint8_t GlobalBufferMemoryManager::mGlobalBuffer[GlobalBufferMemoryManager::BUFFER_SIZE] = {0};
GlobalBufferMemoryManager::FreeBlock* GlobalBufferMemoryManager::mFreeList = nullptr;

size_t alignSize(size_t size) {
  const size_t alignment = 4;  // 4-byte alignment
  return (size + alignment - 1) & ~(alignment - 1);
}

void GlobalBufferMemoryManager::initialize() {
  mFreeList = reinterpret_cast<FreeBlock*>(mGlobalBuffer);
  mFreeList->size = BUFFER_SIZE;
  mFreeList->next = nullptr;
}

void* GlobalBufferMemoryManager::allocate(size_t size) {
  FreeBlock** prev = &mFreeList;
  FreeBlock* block = mFreeList;

  size_t alignedSize = alignSize(size + sizeof(FreeBlock)) - sizeof(FreeBlock);

  // Ensure the block size is at least as large as FreeBlock
  if (alignedSize < sizeof(FreeBlock)) {
    alignedSize = sizeof(FreeBlock);
  }

  UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: Requested size: %d, Adjusted size after alignment: %d", size, alignedSize);

  while (block) {
    UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: Checking block of size %d", block->size);

    if (block->size < sizeof(FreeBlock)) {
      UNIOT_LOG_ERROR("GlobalBufferMemoryManager: Detected a block with invalid size: %d. Possible memory corruption!", block->size);
      return nullptr;
    }

    if (block->size >= alignedSize + sizeof(FreeBlock)) {
      size_t nextBlockSize = block->size - alignedSize - sizeof(FreeBlock);

      // Only split if the next block size is greater than or equal to sizeof(FreeBlock)
      if (nextBlockSize >= sizeof(FreeBlock) && block->size > alignedSize + 2 * sizeof(FreeBlock)) {
        // Split the block
        FreeBlock* nextBlock = reinterpret_cast<FreeBlock*>(reinterpret_cast<uint8_t*>(block) + alignedSize + sizeof(FreeBlock));
        nextBlock->size = nextBlockSize;
        nextBlock->next = block->next;

        block->size = alignedSize;
        block->next = nextBlock;

        // Adjust the previous block to point to the new next block
        *prev = nextBlock;

        UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: Block split done, new block of size %d and next block of size %d", block->size, nextBlock->size);
      } else {
        // If we can't split, adjust the previous block to point to the next one
        *prev = block->next;
      }

      UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: allocated %d bytes", block->size);
      return reinterpret_cast<uint8_t*>(block) + sizeof(FreeBlock);
    }

    prev = &block->next;
    block = block->next;
  }

  // No suitable block was found
  UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: no suitable block was found for %d bytes", alignedSize);
  return nullptr;
}

void GlobalBufferMemoryManager::deallocate(void* ptr) {
  if (!ptr) return;

  FreeBlock* blockToFree = reinterpret_cast<FreeBlock*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(FreeBlock));

  FreeBlock* prev = nullptr;
  FreeBlock* current = mFreeList;

  // Insert the block in its correct position in the sorted free list
  while (current && current < blockToFree) {
    prev = current;
    current = current->next;
  }

  if (prev) {
    // Check if the block to free can be merged with the previous block
    if (reinterpret_cast<uint8_t*>(prev) + prev->size + sizeof(FreeBlock) == reinterpret_cast<uint8_t*>(blockToFree)) {
      prev->size += blockToFree->size + sizeof(FreeBlock);
      blockToFree = prev;
    } else {
      prev->next = blockToFree;
    }
  } else {
    mFreeList = blockToFree;
  }

  // Additional check to ensure that the blocks are truly adjacent in memory before merging
  if (current && reinterpret_cast<uint8_t*>(blockToFree) + blockToFree->size + sizeof(FreeBlock) == reinterpret_cast<uint8_t*>(current)) {
    // Merge blockToFree with current block
    blockToFree->size += current->size + sizeof(FreeBlock);
    blockToFree->next = current->next;
  } else {
    blockToFree->next = current;
  }

  UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: deallocated %d bytes", blockToFree->size - sizeof(FreeBlock));
}

void* GlobalBufferMemoryManager::reallocate(void* ptr, size_t newSize) {
  UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: Entering reallocate. Requested newSize: %d", newSize);

  if (!ptr) {
    UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: ptr is null. Redirecting to allocate.");
    return allocate(newSize);
  }

  FreeBlock* block = reinterpret_cast<FreeBlock*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(FreeBlock));

  UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: Current block size: %d, sizeof(FreeBlock): %d", block->size, sizeof(FreeBlock));

  // Check if block size is a negative value
  if (block->size < 0) {
    UNIOT_LOG_ERROR("GlobalBufferMemoryManager: Detected negative block size: %d. Possible memory corruption!", block->size);
    return nullptr;  // or handle appropriately
  }

  assert(block->size >= sizeof(FreeBlock));

  if (block->size >= newSize) {
    // If the block is big enough, just return the existing pointer
    UNIOT_LOG_DEBUG_IF(DEBUG, "GlobalBufferMemoryManager: Existing block size is sufficient. Exiting reallocate.");
    return ptr;
  }

  // Otherwise, allocate a new block and copy over the old data
  void* newPtr = allocate(newSize);
  if (newPtr) {
    memcpy(newPtr, ptr, block->size);
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
  while (block) {
    totalFree += block->size;
    block = block->next;
  }
  return totalFree;
}

size_t GlobalBufferMemoryManager::getLargestFreeBlock() {
  size_t largestBlock = 0;
  FreeBlock* block = mFreeList;
  while (block) {
    if (block->size > largestBlock) {
      largestBlock = block->size;
    }
    block = block->next;
  }
  return largestBlock;
}

}  // namespace uniot
