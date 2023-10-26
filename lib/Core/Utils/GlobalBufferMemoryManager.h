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

class GlobalBufferMemoryManager {
 public:
  // Initialize the memory manager
  static void initialize();

  // Allocate memory from the global buffer
  static void* allocate(size_t size);

  // Deallocate previously allocated memory
  static void deallocate(void* ptr);

  // Resize allocated memory
  static void* reallocate(void* ptr, size_t newSize);

  // Get the total free memory
  static size_t getTotalFreeMemory();

  // Get the largest free block
  static size_t getLargestFreeBlock();

 private:
  struct FreeBlock {
    size_t size;
    FreeBlock* next;
  };

  // Debug flag
  static constexpr bool DEBUG = false;

  // Size of the global buffer (change as needed)
  static constexpr size_t BUFFER_SIZE = 4096;

  // The global buffer
  static uint8_t mGlobalBuffer[BUFFER_SIZE];

  static FreeBlock* mFreeList;
};
}  // namespace uniot
