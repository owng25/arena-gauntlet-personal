#include "ecs/event.h"
#include "ecs/event_types_data.h"

namespace simulation
{

template <size_t... index>
constexpr bool AllDataTypesMappedImpl(std::index_sequence<index...>)
{
    return (true && ... && event_impl::EventTypeToDataTypeImpl<static_cast<EventType>(index)>::mapped);
}

constexpr bool AllDataTypesMapped()
{
    return AllDataTypesMappedImpl(std::make_index_sequence<static_cast<size_t>(EventType::kNum)>());
}

static_assert(
    AllDataTypesMapped(),
    "Most likely you have added a new event to EventType and did not update mapping in event_types_data.h");
}  // namespace simulation
