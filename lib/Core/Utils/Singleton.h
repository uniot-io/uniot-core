/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2024 Uniot <contact@uniot.io>
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

namespace uniot {

/**
 * @brief Template class implementing the Singleton design pattern.
 * @defgroup utils_singleton Singleton
 * @ingroup utils
 *
 * The Singleton template ensures that a class has only one instance and provides
 * a global point of access to it. This implementation uses the Curiously Recurring
 * Template Pattern (CRTP) where the derived class passes itself as a template parameter.
 *
 * @tparam Derived The class that will become a Singleton
 *
 * Usage example:
 * ```
 * class MyClass : public Singleton<MyClass> {
 *   friend class Singleton<MyClass>; // Required to allow construction
 * private:
 *   MyClass() = default; // Constructor is private
 *
 * public:
 *   void doSomething() { ... }
 * };
 *
 * // Access the singleton instance
 * MyClass::getInstance().doSomething();
 * ```
 *
 * @{
 */
template <typename Derived>
class Singleton {
 public:
  /**
   * @brief Copy constructor is deleted to prevent duplicating the singleton instance.
   */
  Singleton(const Singleton&) = delete;

  /**
   * @brief Assignment operator is deleted to prevent duplicating the singleton instance.
   */
  Singleton& operator=(const Singleton&) = delete;

  /**
   * @brief Gets the singleton instance of the derived class.
   *
   * This method creates the instance on first call (lazy initialization) and
   * returns a reference to it on all subsequent calls. The instance is created
   * as a static variable, ensuring thread-safe initialization in C++11 and above.
   *
   * @retval Derived& Reference to the singleton instance of the derived class
   */
  static inline Derived& getInstance() {
    static Derived instance;
    return instance;
  }

 protected:
  /**
   * @brief Default constructor is protected to prevent instantiation outside of derived classes.
   *
   * Derived classes should declare Singleton as a friend class to allow instantiation
   * while keeping their constructors protected or private.
   */
  Singleton() = default;

  /**
   * @brief Default destructor is protected to prevent deletion through base class pointer.
   */
  ~Singleton() = default;
};
/** @} */
}  // namespace uniot
