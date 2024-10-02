#pragma once

namespace uniot {
typedef const void* type_id;

class IWithType {
 public:
  virtual type_id getTypeId() const = 0;
};

class Type {
 public:
  template <class T>
  static inline type_id getTypeId() {
    static T* TypeUniqueMarker = nullptr;
    return &TypeUniqueMarker;
  }

  template <typename T>
  static inline T* safeStaticCast(IWithType* obj) {
    if (obj->getTypeId() == Type::getTypeId<T>()) {
      return static_cast<T*>(obj);
    }
    UNIOT_LOG_DEBUG("cast failed from [%lu] to [%lu]", obj->getTypeId(), Type::getTypeId<T>());
    return nullptr;
  }
};
}
