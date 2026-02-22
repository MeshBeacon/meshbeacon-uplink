# Fix: IQ Inversion on CDP Downlinks

## Problem

MamaDuck devices were never receiving RREP (Route Response) or any
downlink packets from the gateway (`clusterduckd`), despite the gateway
successfully transmitting them. The MamaDuck would always time out after
80 seconds and join the public network without a valid routing table,
resulting in garbage DDUIDs in uplink packets.

## Root Cause

In `clusterduckd.c`, the CDP downlink TX packet was configured with:

```c
txpkt.invert_pol = true;
```

This is a **LoRaWAN convention**. In LoRaWAN, gateways invert the IQ
phase on downlinks so that end devices do not hear other end devices'
transmissions (end devices transmit with normal IQ, gateways downlink
with inverted IQ).

**ClusterDuck Protocol (CDP) is not LoRaWAN.** The MamaDuck's SX1262
radio is configured by RadioLib with normal (non-inverted) IQ. When the
gateway transmitted a RREP with `invert_pol = true`, the MamaDuck's
SX1262 could not decode it — the packet appeared as noise. No CRC error
was flagged because the preamble itself was not detected due to the IQ
mismatch.

## Evidence

Gateway log showed RREP being transmitted and immediately heard back via
self-loopback (rssi≈-17, freq=923.6MHz — a known SX1302 self-image
artifact). MamaDuck serial showed no `packet reception complete`
interrupt after any RREP, only after its own TX_DONE. This continued
through all 5 RREQ attempts until the 80-second timeout.

## Fix

Changed `invert_pol` to `false` for CDP downlinks in `clusterduckd.c`:

```c
/* CDP is not LoRaWAN — MamaDuck uses normal (non-inverted) IQ */
txpkt.invert_pol = false;
```

After this fix, the MamaDuck received the RREP on the first attempt:

```
SX1262 Interrupt flag was set: packet reception complete
readReceivedData() - packet length returns: 93
Rx packet: PAPADUCKZAIHAN12...
[ROUTER] Public network joined
```

## Affected File

- `clusterduck/src/clusterduckd.c` — inner downlink loop, `txpkt.invert_pol`

## Related Fixes Applied in the Same Session

| Fix | File | Description |
|-----|------|-------------|
| `invert_pol = false` | `clusterduckd.c` | **Primary fix** — allows MamaDuck to decode downlinks |
| TX Gain LUT (SX1250 format) | `global_conf.json` | Removed `WARNING: No TX gain LUT defined for rf_chain 0` |
| 500ms delay before RREP TX | `clusterduckd.c` | Safety margin for SX1262 TX→RX turnaround after RREQ |
| Reply on same RX frequency | `DuckLoRa.cpp` + `ClusterDuckBridge.cpp` | RREP transmitted on exact same channel RREQ was received on |
| Dedup window 30s | `clusterduckd.c` | Prevents duplicate command delivery within 30 seconds |
| SEARCHING-state packet buffering | `Duck.h` / `MamaDuck.h` | Buffers packets received before MamaDuck goes PUBLIC |

## Note for Future Development

Any downlink packet sent from `clusterduckd` to a CDP device (MamaDuck,
DuckLink, etc.) **must use `invert_pol = false`**. Only use
`invert_pol = true` if the target device is a standard LoRaWAN end
device expecting inverted IQ downlinks.
