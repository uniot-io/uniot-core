#pragma once

typedef const void* TypeId;

template <class T>
TypeId GetTypeId() {
  static T* TypeUniqueMarker = nullptr;
  return &TypeUniqueMarker;
}
