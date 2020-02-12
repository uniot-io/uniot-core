/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <contact@uniot.io>
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

#include "CallbackSubscriber.h"

namespace uniot
{
template <class T_topic, class T_msg>
CallbackSubscriber<T_topic, T_msg>::CallbackSubscriber(SubscriberCallback callback)
    : mCallback(callback) {}

template <class T_topic, class T_msg>
void CallbackSubscriber<T_topic, T_msg>::onPublish(T_topic topic, T_msg msg)
{
  mCallback(topic, msg);
}
} // namespace uniot

template class uniot::CallbackSubscriber<unsigned int, int>;