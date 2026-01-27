#include "duck_bridge.h"

#include <mutex>
#include <deque>
#include <vector>
#include <cstring>

// Small, thread-safe in-memory queue for downlinks. This is intentionally minimal.
// Tune MAX_DOWNLINK_QUEUE or implement persistence if needed.
struct Downlink {
    std::vector<uint8_t> payload;
    uint16_t size;
    uint32_t freq_hz;
    uint32_t tmst;
    int16_t tx_power_dbm;
    uint32_t bandwidth_hz;
    uint8_t datarate_sf;
    uint8_t coderate;
    uint8_t rf_chain;
};

static std::mutex g_mutex;
static std::deque<Downlink> g_downlink_queue;
static const size_t MAX_DOWNLINK_QUEUE = 8;

// RX callback (optional) that DuckLoRa can register to be notified immediately
static duck_rx_cb_t g_rx_callback = nullptr;

extern "C" int duck_handle_gateway_rx(const uint8_t* payload, uint16_t size,
                                      int16_t rssi, float snr,
                                      uint32_t freq_hz, uint32_t tmst, uint8_t rf_chain,
                                      uint32_t bandwidth_hz, uint8_t datarate_sf, uint8_t coderate)
{
    if (!payload || size == 0) return -1;

    // If a callback is registered, deliver immediately
    duck_rx_cb_t cb = nullptr;
    {
        std::lock_guard<std::mutex> lg(g_mutex);
        cb = g_rx_callback;
    }
    if (cb) {
        cb(payload, size, rssi, snr, freq_hz, tmst, rf_chain, bandwidth_hz, datarate_sf, coderate);
        return 0;
    }

    // No callback => nothing else to do here for now.
    // Optionally, you can add a small RX queue here for later inspection.
    (void)rssi; (void)snr; (void)freq_hz; (void)tmst; (void)rf_chain; (void)bandwidth_hz; (void)datarate_sf; (void)coderate;
    return 0;
}

extern "C" void duck_register_rx_callback(duck_rx_cb_t cb)
{
    std::lock_guard<std::mutex> lg(g_mutex);
    g_rx_callback = cb;
}

/*extern "C" int duck_enqueue_downlink(const uint8_t* payload, uint16_t size,
                                     uint32_t freq_hz, uint32_t tmst, int16_t tx_power_dbm,
                                     uint32_t bandwidth_hz, uint8_t datarate_sf, uint8_t coderate,
                                     uint8_t rf_chain)
{
    if (!payload || size == 0) return -1;

    std::lock_guard<std::mutex> lg(g_mutex);
    if (g_downlink_queue.size() >= MAX_DOWNLINK_QUEUE) {
        // Queue full: drop oldest to make room (policy can be adjusted)
        g_downlink_queue.pop_front();
    }
    Downlink d;
    d.payload.assign(payload, payload + size);
    d.size = size;
    d.freq_hz = freq_hz;
    d.tmst = tmst;
    d.tx_power_dbm = tx_power_dbm;
    d.bandwidth_hz = bandwidth_hz;
    d.datarate_sf = datarate_sf;
    d.coderate = coderate;
    d.rf_chain = rf_chain;
    g_downlink_queue.push_back(std::move(d));
    return 0;
}
*/
extern "C" int duck_enqueue_downlink(const uint8_t* payload, uint16_t size)
{
    if (!payload || size == 0) return -1;

    std::lock_guard<std::mutex> lg(g_mutex);
    if (g_downlink_queue.size() >= MAX_DOWNLINK_QUEUE) {
        // Queue full: drop oldest to make room (policy can be adjusted)
        g_downlink_queue.pop_front();
    }
    Downlink d;
    d.payload.assign(payload, payload + size);
    d.size = size;
    return 0;
}


extern "C" int duck_pop_downlink(uint8_t* payload_out, uint16_t* size_out,
                                 uint32_t* freq_hz, uint32_t* tmst, int16_t* tx_power_dbm,
                                 uint32_t* bandwidth_hz, uint8_t* datarate_sf, uint8_t* coderate,
                                 uint8_t* rf_chain)
{
    std::lock_guard<std::mutex> lg(g_mutex);
    if (g_downlink_queue.empty()) return -1;
    Downlink d = std::move(g_downlink_queue.front());
    g_downlink_queue.pop_front();

    if (payload_out && size_out) {
        // caller must pass a buffer large enough (size_out is in/out: initially holds buffer capacity)
        if (*size_out < d.size) {
            // Insufficient buffer capacity: communicate required size by setting *size_out = 0
            *size_out = 0;
            return -2;
        }
        std::memcpy(payload_out, d.payload.data(), d.size);
        *size_out = d.size;
    }
    if (freq_hz) *freq_hz = d.freq_hz;
    if (tmst) *tmst = d.tmst;
    if (tx_power_dbm) *tx_power_dbm = d.tx_power_dbm;
    if (bandwidth_hz) *bandwidth_hz = d.bandwidth_hz;
    if (datarate_sf) *datarate_sf = d.datarate_sf;
    if (coderate) *coderate = d.coderate;
    if (rf_chain) *rf_chain = d.rf_chain;
    return 0;
}

extern "C" int duck_has_downlink(void)
{
    std::lock_guard<std::mutex> lg(g_mutex);
    return g_downlink_queue.empty() ? 0 : 1;
}
