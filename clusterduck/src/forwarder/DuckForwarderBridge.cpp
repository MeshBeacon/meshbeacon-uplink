#include "DuckForwarderBridge.h"
#include <mutex>

namespace duck_forwarder_bridge {

static std::mutex s_mutex;
static std::optional<std::vector<uint8_t>> s_packet;

void push_packet(const uint8_t* payload, uint16_t size,
                 int16_t /*rssi*/, float /*snr*/,
                 uint32_t /*freq_hz*/, uint32_t /*tmst*/, uint8_t /*rf_chain*/,
                 uint32_t /*bandwidth_hz*/, uint8_t /*datarate_sf*/, uint8_t /*coderate*/)
{
    if (payload == nullptr || size == 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(s_mutex);
    s_packet = std::vector<uint8_t>(payload, payload + size);
}

std::optional<std::vector<uint8_t>> pop_packet()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_packet.has_value()) {
        return std::nullopt;
    }
    auto out = std::move(s_packet.value());
    s_packet.reset();
    return out;
}

bool has_packet()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_packet.has_value();
}

} // namespace duck_forwarder_bridge
