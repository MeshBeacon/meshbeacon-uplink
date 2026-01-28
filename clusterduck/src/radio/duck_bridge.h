#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*duck_rx_cb_t)(const uint8_t* payload, uint16_t size,
                             int16_t rssi, float snr,
                             uint32_t freq_hz, uint32_t tmst, uint8_t rf_chain,
                             uint32_t bandwidth_hz, uint8_t datarate_sf, uint8_t coderate);

/* RX registration and notify (forwarder -> DuckLoRa) */
void duck_register_rx_callback(duck_rx_cb_t cb);
void duck_handle_gateway_rx(const uint8_t* payload, uint16_t size,
                            int16_t rssi, float snr,
                            uint32_t freq_hz, uint32_t tmst, uint8_t rf_chain,
                            uint32_t bandwidth_hz, uint8_t datarate_sf, uint8_t coderate);

/* TX enqueue (DuckLoRa -> forwarder) */
int duck_enqueue_downlink(const uint8_t* payload, uint16_t size);
/* Optional extended version with metadata; any zero means “unspecified/default” */
int duck_enqueue_downlink_ext(const uint8_t* payload, uint16_t size,
                              uint32_t freq_hz, uint32_t tmst, int16_t tx_power_dbm,
                              uint32_t bw_hz, uint8_t sf, uint8_t cr, uint8_t rf_chain);

/* Forwarder side pop (clusterduckd.c) */
int duck_pop_downlink(uint8_t* out_buf, uint16_t* inout_capacity,
                      uint32_t* freq_hz, uint32_t* tmst, int16_t* tx_power_dbm,
                      uint32_t* bw_hz, uint8_t* sf, uint8_t* cr, uint8_t* rf_chain);

#ifdef __cplusplus
}
#endif
