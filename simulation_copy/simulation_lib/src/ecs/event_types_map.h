#pragma once

#include "event.h"

namespace simulation::event_impl
{

// This is a forward declaration for event type map (which is defined in event_types_data.h)
template <EventType event_type>
struct EventTypeToDataTypeImpl;

template <EventType event_type>
using EventTypeToDataType = typename EventTypeToDataTypeImpl<event_type>::Type;
}  // namespace simulation::event_impl
