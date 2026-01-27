#ifndef DUCK_BRIDGE_H
#define DUCK_BRIDGE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Callback type that DuckLoRa (C++) can register to be notified when the forwarder
// receives a concentrator RX packet. The parameters mirror typical lgw_pkt_rx_s fields.
//
// payload - pointer to bytes (not null-terminated)
// size - payload size in bytes
// rssi - RSSI in dBm (signed 16-bit)
// snr - SNR in dB (float)
// freq_hz - frequency in Hz (uint32_t)
// tmst - concentrator timestamp (count_us) if available (uint32_t) or 0
// rf_chain - IF/rf chain index (uint8_t)
// bandwidth_hz - bandwidth in Hz (uint32_t)
// datarate_sf - spreading factor (6..12) for LoRa (uint8_t)
// coderate - coding rate small integer (e.g. 1..4) (uint8_t)
typedef void (*duck_rx_cb_t)(const uint8_t* payload, uint16_t size,
                             int16_t rssi, float snr,
                             uint32_t freq_hz, uint32_t tmst, uint8_t rf_chain,
                             uint32_t bandwidth_hz, uint8_t datarate_sf, uint8_t coderate);

// Called by packet_forwarder when a lgw_pkt_rx_s packet is available.
// Non-blocking. Returns 0 on success, negative on error.
int duck_handle_gateway_rx(const uint8_t* payload, uint16_t size,
                           int16_t rssi, float snr,
                           uint32_t freq_hz, uint32_t tmst, uint8_t rf_chain,
                           uint32_t bandwidth_hz, uint8_t datarate_sf, uint8_t coderate);

// Register a C callback so DuckLoRa/C++ can be notified directly when forwarder receives a packet.
// If a callback is registered, duck_handle_gateway_rx delivers the RX to it immediately.
void duck_register_rx_callback(duck_rx_cb_t cb);

// Enqueue a downlink from ClusterDuck (DuckLoRa or other component) for the forwarder to pick up.
// Non-blocking. Returns 0 on success, negative on error.
// Fields are the minimal ones threads_down will need to fill lgw_pkt_tx_s.
/*int duck_enqueue_downlink(const uint8_t* payload, uint16_t size,
                          uint32_t freq_hz, uint32_t tmst, int16_t tx_power_dbm,
                          uint32_t bandwidth_hz, uint8_t datarate_sf, uint8_t coderate,
                          uint8_t rf_chain);*/
int duck_enqueue_downlink(const uint8_t* payload, uint16_t size);

// Pop the next pending downlink. threads_down should call this to obtain a downlink to transmit.
// Non-blocking. On success returns 0 and fills callers' pointers. If no downlink queued returns -1.
// For payload_out the caller must pass a pointer and *size_out must contain the buffer capacity on call,
// on return *size_out will contain the actual payload size.
int duck_pop_downlink(uint8_t* payload_out, uint16_t* size_out,
                      uint32_t* freq_hz, uint32_t* tmst, int16_t* tx_power_dbm,
                      uint32_t* bandwidth_hz, uint8_t* datarate_sf, uint8_t* coderate,
                      uint8_t* rf_chain);

// Convenience: check whether a downlink is pending (non-blocking). Returns 1 if pending, 0 if none.
int duck_has_downlink(void);

#ifdef __cplusplus
}
#endif

#endif // DUCK_BRIDGE_H
