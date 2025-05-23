#pragma once

#include <Arduino.h>

namespace uniot {
namespace JSON {

class Array;

class Object {
 public:
  Object(String &out) : mOut(out), mFirst(true) {
    mOut += "{";
  }

  inline Object &put(const String &key, const String &value, bool quote = true) {
    _begin(key);
    mOut += (quote ? "\"" + value + "\"" : value);
    return *this;
  }

  inline Object &put(const String &key, int value) {
    return put(key, String(value), false);
  }

  inline Array putArray(const String &key);
  inline Object putObject(const String &key);

  inline void close() {
    mOut += "}";
  }

 private:
  String &mOut;
  bool mFirst;

  inline void _begin(const String &key) {
    if (!mFirst) {
      mOut += ",";
    }
    mOut += "\"" + key + "\":";
    mFirst = false;
  }
};

class Array {
 public:
  Array(String &out) : mOut(out), mFirst(true) {
    mOut += "[";
  }

  inline Array &append(const String &value, bool quote = true) {
    _begin();
    mOut += (quote ? "\"" + value + "\"" : value);
    return *this;
  }

  inline Array &append(int value) {
    return append(String(value), false);
  }

  inline Array appendArray();
  Object appendObject();

  inline void close() {
    mOut += "]";
  }

 private:
  String &mOut;
  bool mFirst;

  inline void _begin() {
    if (!mFirst) mOut += ",";
    mFirst = false;
  }
};

inline Array Object::putArray(const String &key) {
  _begin(key);
  return Array(mOut);
}

inline Object Object::putObject(const String &key) {
  _begin(key);
  return Object(mOut);
}

inline Array Array::appendArray() {
  _begin();
  return Array(mOut);
}

inline Object Array::appendObject() {
  _begin();
  return Object(mOut);
}

}  // namespace JSON
}  // namespace uniot