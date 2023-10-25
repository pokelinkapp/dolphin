// Copyright 2018 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <mutex>
#include <map>
#include <ranges>

#include "Common/Assert.h"
#include "Core/Core.h"
#include "Common/CommonTypes.h"
#include "Common/Logging/Log.h"

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

  bool operator==(const ListenerID& o) const { return value == o.value; }
  bool operator<(const ListenerID& o) const { return value < o.value; }
};

// an event container manages a single event type
template <typename T>
class EventContainer final
{
public:
  bool HasListeners()
  {
    return !m_listeners.empty();
  }

  void EmitEvent(T evt)
  {
    // Some events are not necessarily produced within the CPU thread,
    // e.g. FrameDrawn originates from the FrameDumper thread.
    // However, we cannot have concurrent Python code invocations,
    // because Python code might invoke Dolphin code that requires
    // a CPU thread lock, but concurrent events on the CPU thread
    // need the Python GIL to emit their events. This can lead to
    // deadlocks between the CPU thread guard and the GIL.
    // See for example https://github.com/Felk/dolphin/issues/25#issuecomment-1736209834
    // That's why every event must be emitted from the CPU thread,
    // and all event sources are responsible to schedule their events
    // into the emulation somehow.
    ASSERT_MSG(SCRIPTING, Core::IsCPUThread(),
               "Events must be emitted from the CPU thread, but {} wasn't", typeid(T).name());

    // Events are processed sequentially due to the fact that they are
    // happening on the CPU thread, but Python code could theoretically
    // spawn new host threads for example to do stuff concurrently.
    // Just to be sure, have some guards against concurrent modifications.
    std::lock_guard lock{m_listeners_iterate_mutex};
    // avoid concurrent modification issues by iterating over a copy
    auto view = m_listeners | std::views::values;
    for (const auto copy = std::vector<Listener<T>>(view.begin(), view.end());
         const Listener<T>& listener : copy)
    {
      listener(evt);
    }
  }

  ListenerID<T> ListenEvent(Listener<T> listener)
  {
    auto id = ListenerID<T>{m_next_listener_id++};
    m_listeners[id] = std::move(listener);
    return id;
  }

  bool UnlistenEvent(ListenerID<T> listener_id)
  {
    return m_listeners.erase(listener_id) > 0;
  }

  void TickListeners()
  {
    std::lock_guard lock{m_listeners_iterate_mutex};
  }
private:
  std::mutex m_listeners_iterate_mutex{};
  std::map<ListenerID<T>, Listener<T>> m_listeners{};
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
