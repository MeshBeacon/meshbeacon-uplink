#include "duck_bridge.h"
#include <vector>
#include <deque>
#include <mutex>
#include <cstring>
#include <cstdio>

struct DuckDlItem {
    std::vector<uint8_t> payload;
    uint32_t freq_hz = 0;
    uint32_t tmst = 0;
    int16_t  tx_power_dbm = 0;
    uint32_t bw_hz = 0;
    uint8_t  sf = 0;
    uint8_t  cr = 0;
    uint8_t  rf_chain = 0;
};

static std::mutex g_mx;
static std::deque<DuckDlItem> g_q;

static duck_rx_cb_t g_rx_cb = nullptr;

extern "C" {

void duck_register_rx_callback(duck_rx_cb_t cb) { 
    g_rx_cb = cb; 
    printf("[DUCK_BRIDGE] RX callback %s (cb=%p)\n", 
           cb ? "REGISTERED" : "UNREGISTERED", (void*)cb);
}

void duck_handle_gateway_rx(const uint8_t* payload, uint16_t size,
                            int16_t rssi, float snr,
                            uint32_t freq_hz, uint32_t tmst, uint8_t rf_chain,
                            uint32_t bandwidth_hz, uint8_t datarate_sf, uint8_t coderate) {
    printf("[DUCK_BRIDGE] Received packet: size=%u, rssi=%d, snr=%.1f, callback=%s\n", 
           size, rssi, snr, g_rx_cb ? "registered" : "NULL");
    
    if (g_rx_cb) {
        g_rx_cb(payload, size, rssi, snr, freq_hz, tmst, rf_chain,
                bandwidth_hz, datarate_sf, coderate);
    } else {
        printf("[DUCK_BRIDGE] WARNING: No RX callback registered!\n");
    }
}

int duck_enqueue_downlink(const uint8_t* payload, uint16_t size) {
    if (!payload || size == 0) return -1;
    DuckDlItem it;
    it.payload.assign(payload, payload + size);
    std::lock_guard<std::mutex> lk(g_mx);
    g_q.emplace_back(std::move(it));
    return 0;
}

int duck_enqueue_downlink_ext(const uint8_t* payload, uint16_t size,
                              uint32_t freq_hz, uint32_t tmst, int16_t tx_power_dbm,
                              uint32_t bw_hz, uint8_t sf, uint8_t cr, uint8_t rf_chain) {
    if (!payload || size == 0) return -1;
    DuckDlItem it;
    it.payload.assign(payload, payload + size);
    it.freq_hz = freq_hz;
    it.tmst = tmst;
    it.tx_power_dbm = tx_power_dbm;
    it.bw_hz = bw_hz;
    it.sf = sf;
    it.cr = cr;
    it.rf_chain = rf_chain;
    std::lock_guard<std::mutex> lk(g_mx);
    g_q.emplace_back(std::move(it));
    return 0;
}

int duck_pop_downlink(uint8_t* out_buf, uint16_t* inout_capacity,
                      uint32_t* freq_hz, uint32_t* tmst, int16_t* tx_power_dbm,
                      uint32_t* bw_hz, uint8_t* sf, uint8_t* cr, uint8_t* rf_chain) {
    if (!inout_capacity) return -1;
    std::lock_guard<std::mutex> lk(g_mx);
    if (g_q.empty()) return 1;

    DuckDlItem it = std::move(g_q.front());
    g_q.pop_front();

    if (!out_buf || *inout_capacity < it.payload.size()) {
        *inout_capacity = 0;
        return -2;
    }
    std::memcpy(out_buf, it.payload.data(), it.payload.size());
    *inout_capacity = static_cast<uint16_t>(it.payload.size());

    if (freq_hz)      *freq_hz = it.freq_hz;
    if (tmst)         *tmst = it.tmst;
    if (tx_power_dbm) *tx_power_dbm = it.tx_power_dbm;
    if (bw_hz)        *bw_hz = it.bw_hz;
    if (sf)           *sf = it.sf;
    if (cr)           *cr = it.cr;
    if (rf_chain)     *rf_chain = it.rf_chain;

    return 0;
}

} // extern "C"
