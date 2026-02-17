/**
 * @file duck_bridge_compat.h
 * @brief Backward compatibility layer for old duck_bridge API
 * 
 * This file provides compatibility aliases for code that still uses
 * the old duck_bridge.h interface. New code should use ClusterDuckBridge.h.
 * 
 * @deprecated Use ClusterDuckBridge.h instead
 */

#pragma once

#include "ClusterDuckBridge.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Backward compatibility type aliases */
typedef cdp_rx_callback_t duck_rx_cb_t;

/* Backward compatibility function aliases */
static inline void duck_register_rx_callback(duck_rx_cb_t cb) {
    cdp_bridge_register_rx_callback(cb);
}

static inline void duck_handle_gateway_rx(
    const uint8_t* payload, uint16_t size,
    int16_t rssi, float snr,
    uint32_t freq_hz, uint32_t tmst, uint8_t rf_chain,
    uint32_t bandwidth_hz, uint8_t datarate_sf, uint8_t coderate)
{
    cdp_bridge_handle_uplink(payload, size, rssi, snr, freq_hz, tmst, rf_chain,
                             bandwidth_hz, datarate_sf, coderate);
}

static inline int duck_enqueue_downlink(const uint8_t* payload, uint16_t size) {
    return cdp_bridge_enqueue_downlink(payload, size);
}

static inline int duck_enqueue_downlink_ext(
    const uint8_t* payload, uint16_t size,
    uint32_t freq_hz, uint32_t tmst, int16_t tx_power_dbm,
    uint32_t bw_hz, uint8_t sf, uint8_t cr, uint8_t rf_chain)
{
    return cdp_bridge_enqueue_downlink_ext(payload, size, freq_hz, tmst, tx_power_dbm,
                                           bw_hz, sf, cr, rf_chain);
}

static inline int duck_pop_downlink(
    uint8_t* out_buf, uint16_t* inout_capacity,
    uint32_t* freq_hz, uint32_t* tmst, int16_t* tx_power_dbm,
    uint32_t* bw_hz, uint8_t* sf, uint8_t* cr, uint8_t* rf_chain)
{
    return cdp_bridge_pop_downlink(out_buf, inout_capacity, freq_hz, tmst, tx_power_dbm,
                                   bw_hz, sf, cr, rf_chain);
}

#ifdef __cplusplus
}
#endif

/* Backward compatibility for DuckForwarderBridge namespace */
#ifdef __cplusplus
namespace duck_forwarder_bridge {
    using cdp_bridge::has_uplink_packet;
    using cdp_bridge::pop_uplink_packet;
    
    static inline void push_packet(
        const uint8_t* payload, uint16_t size,
        int16_t rssi, float snr,
        uint32_t freq_hz, uint32_t tmst, uint8_t rf_chain,
        uint32_t bandwidth_hz, uint8_t datarate_sf, uint8_t coderate)
    {
        cdp_bridge_handle_uplink(payload, size, rssi, snr, freq_hz, tmst, rf_chain,
                                bandwidth_hz, datarate_sf, coderate);
    }
    
    static inline std::optional<std::vector<uint8_t>> pop_packet() {
        return cdp_bridge::pop_uplink_packet();
    }
    
    static inline bool has_packet() {
        return cdp_bridge::has_uplink_packet();
    }
}
#endif
