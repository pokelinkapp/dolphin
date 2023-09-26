// Copyright 2018 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <mutex>

#include "Core/Core.h"
#include "Common/CommonTypes.h"

namespace API
{
namespace Events
{
// events are defined as structs.
// each event also has to be added to the EventHub type alias.
struct FrameAdvance
{
};
struct FrameDrawn
{
  int width;
  int height;
  const u8* data;
};
struct MemoryBreakpoint
{
  bool write;
  u32 addr;
  u64 value;
};
struct CodeBreakpoint
{
  u32 addr;
};
struct SetInterrupt
{
  u32 cause_mask;
};
struct ClearInterrupt
{
  u32 cause_mask;
};

}  // namespace API::Events

// a listener on T is any function that takes a const reference to T as argument
template <typename T>
using Listener = std::function<void(const T&)>;
template <typename T>
using ListenerFuncPtr = void (*)(const T&);

// inside a generic struct to make listener IDs typesafe per event
template <typename T>
struct ListenerID
{
  u64 value;
};

// an event container manages a single event type
template <typename T>
class EventContainer final
{
public:
  bool HasListeners()
  {
    return !m_listener_pairs.empty() || !m_one_time_listeners.empty();
  }

  void EmitEvent(T evt)
  {
    // Events are not necessarily emitted from the CPU thread,
    // e.g. FrameDrawn is emitted from the FrameDumper thread.
    // However, we cannot have concurrent Python code invocations,
    // because Python code might invoke Dolphin code that requires
    // a CPU thread lock, but concurrent events on the CPU thread
    // need the Python GIL to emit their events. This can lead to
    // deadlocks between the CPU thread guard and the GIL.
    // See for example https://github.com/Felk/dolphin/issues/25#issuecomment-1736209834
    // That's why we just put every event on the CPU thread.
    Core::RunOnCPUThread(
        [&] {
          // TODO felk: since all events are on the CPU thread now, are these concurrency precautions even necessary anymore?
          // TODO felk: alternatively, would only wrapping each individual invocation with 'RunOnCPUThread' be better/faster?
          std::lock_guard lock{m_listeners_iterate_mutex};
          // avoid concurrent modification issues by iterating over a copy
          std::vector<std::pair<ListenerID<T>, Listener<T>>> listener_pairs = m_listener_pairs;
          for (auto& listener_pair : listener_pairs)
            listener_pair.second(evt);
          // avoid concurrent modification issues by performing a swap
          // with an fresh empty vector.
          std::vector<Listener<T>> one_time_listeners;
          std::swap(one_time_listeners, m_one_time_listeners);
          for (auto& listener : one_time_listeners)
            listener(evt);
        },
        true);
  }

  ListenerID<T> ListenEvent(Listener<T> listener)
  {
    auto id = ListenerID<T>{m_next_listener_id++};
    m_listener_pairs.emplace_back(std::pair<ListenerID<T>, Listener<T>>(id, std::move(listener)));
    return id;
  }

  bool UnlistenEvent(ListenerID<T> listener_id)
  {
    for (auto it = m_listener_pairs.begin(); it != m_listener_pairs.end(); ++it)
    {
      if (it->first.value == listener_id.value)
      {
        m_listener_pairs.erase(it);
        return true;
      }
    }
    return false;
  }

  void ListenEventOnce(Listener<T> listener)
  {
    m_one_time_listeners.emplace_back(std::move(listener));
  }

  void TickListeners()
  {
    std::lock_guard lock{m_listeners_iterate_mutex};
  }
private:
  std::mutex m_listeners_iterate_mutex{};
  std::vector<std::pair<ListenerID<T>, Listener<T>>> m_listener_pairs{};
  std::vector<Listener<T>> m_one_time_listeners{};
  u64 m_next_listener_id = 0;
};

// an event hub manages a set of event containers,
// hence being the gateway to a multitude of events
template <typename... Ts>
class GenericEventHub final
{
public:
  template <typename T>
  bool HasListeners()
  {
    return GetEventContainer<T>().HasListeners();
  }

  template <typename T>
  void EmitEvent(T evt)
  {
    GetEventContainer<T>().EmitEvent(evt);
  }

  template <typename T>
  ListenerID<T> ListenEvent(Listener<T> listener)
  {
    return GetEventContainer<T>().ListenEvent(listener);
  }

  // convenience overload
  template <typename T>
  ListenerID<T> ListenEvent(ListenerFuncPtr<T> listener_func_ptr)
  {
    return GetEventContainer<T>().ListenEvent(listener_func_ptr);
  }

  template <typename T>
  bool UnlistenEvent(ListenerID<T> listener_id)
  {
    return GetEventContainer<T>().UnlistenEvent(listener_id);
  }

  template <typename T>
  void ListenEventOnce(Listener<T> listener)
  {
    GetEventContainer<T>().ListenEventOnce(listener);
  }

  void TickAllListeners()
  {
    std::apply([](auto&&... arg) { (arg.TickListeners(), ...); }, m_event_containers);
  }

private:
  template <typename T>
  EventContainer<T>& GetEventContainer()
  {
    return std::get<EventContainer<T>>(m_event_containers);
  }

  std::tuple<EventContainer<Ts>...> m_event_containers;
};

// all existing events need to be listed here, otherwise there will be spooky templating errors
using EventHub = GenericEventHub<Events::FrameAdvance, Events::FrameDrawn, Events::SetInterrupt, Events::ClearInterrupt,
                                 Events::MemoryBreakpoint, Events::CodeBreakpoint>;

// global event hub
EventHub& GetEventHub();

}  // namespace API
