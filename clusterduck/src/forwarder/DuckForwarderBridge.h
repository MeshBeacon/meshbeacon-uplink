#pragma once

#include <cstdint>
#include <vector>
#include <optional>

namespace duck_forwarder_bridge {

/**
 * Push a packet received from the forwarder callback into the bridge.
 * Thread-safe.
 */
void push_packet(const uint8_t* payload, uint16_t size,
                 int16_t rssi, float snr,
                 uint32_t freq_hz, uint32_t tmst, uint8_t rf_chain,
                 uint32_t bandwidth_hz, uint8_t datarate_sf, uint8_t coderate);

/**
 * Pop the most recent packet pushed by the forwarder callback.
 * Returns std::nullopt if no forwarder packet is available.
 * Thread-safe.
 */
std::optional<std::vector<uint8_t>> pop_packet();

/**
 * Return true if a forwarder packet is waiting (non-destructive).
 */
bool has_packet();

} // namespace duck_forwarder_bridge
