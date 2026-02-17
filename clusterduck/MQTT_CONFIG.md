# MQTT Configuration for ClusterDuck "SuperDuck" Gateway

## Overview

The ClusterDuck gateway (`clusterduckd`) acts as a PapaDuck that receives packets from the mesh network and forwards them to an MQTT broker. This configuration follows the standard ClusterDuck Protocol PapaDuck MQTT integration pattern.

## Configuration

MQTT settings are configured in the `global_conf.json` file under the `mqtt_conf` object. This matches the configuration pattern used in the official ClusterDuck Protocol PapaDuck examples.

### Example Configuration (Basic)

```json
{
  "mqtt_conf": {
    "enabled": true,
    "server": "test.mosquitto.org",
    "port": 1883,
    "client_id": "papa-duck-mqtt-1",
    "pub_topic": "hub/event",
    "sub_topic": "incoming/say_hello",
    "keepalive": 60
  }
}
```

### Example Configuration (with TLS)

```json
{
  "mqtt_conf": {
    "enabled": true,
    "server": "test.mosquitto.org",
    "port": 8883,
    "client_id": "papa-duck-mqtt-1",
    "username": "",
    "password": "",
    "pub_topic": "hub/event",
    "sub_topic": "incoming/say_hello",
    "keepalive": 60,
    "use_tls": true,
    "ca_cert_file": "/etc/ssl/certs/ca-certificates.crt"
  }
}
```

### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `enabled` | boolean | `false` | Enable/disable MQTT publishing |
| `server` | string | `""` | MQTT broker hostname or IP address |
| `port` | integer | `1883` | MQTT broker port (1883 for plain, 8883 for TLS) |
| `client_id` | string | `"papa-duck-mqtt-1"` | Unique MQTT client identifier (must be unique per broker) |
| `username` | string | `""` | MQTT authentication username (optional) |
| `password` | string | `""` | MQTT authentication password (optional) |
| `pub_topic` | string | `"hub/event"` | Topic where PapaDuck publishes CDP messages |
| `sub_topic` | string | `"incoming/say_hello"` | Topic for receiving downlink commands (optional) |
| `keepalive` | integer | `60` | MQTT keepalive interval in seconds |
| `use_tls` | boolean | `false` | Enable TLS/SSL encryption |
| `ca_cert_file` | string | `""` | Path to CA certificate file for TLS verification |

**Note**: The `pub_topic` and `sub_topic` values match the standard ClusterDuck Protocol PapaDuck MQTT topics. The PapaDuck publishes all CDP messages to `hub/event` with the CDP topic embedded in the `eventType` field.

## Message Format

The gateway publishes messages in the standard ClusterDuck Protocol PapaDuck format. All CDP messages are published to the `hub/event` topic with the CDP topic name in the `eventType` field.

### Standard PapaDuck Message Format

```json
{
  "from": "hub",
  "to": "controller",
  "RE": false,
  "eventType": "status",
  "MessageID": "Z18L",
  "payload": {
    "hops": 0,
    "duckType": 2,
    "DeviceID": "MAMADUCK",
    "Message": "C:149|FM:366872"
  }
}
```

### Message Fields

- **from**: Always `"hub"` (the PapaDuck gateway)
- **to**: Target recipient, typically `"controller"` 
- **RE**: Response expected flag (boolean)
- **eventType**: CDP topic name (e.g., `"status"`, `"sensor"`, `"alert"`, `"ping"`, `"pong"`)
- **MessageID**: Unique message identifier (MUID) - 4 character string
- **payload**:
  - **hops**: Number of mesh hops the packet traveled
  - **duckType**: Source device type:
    - `1` = DuckLink
    - `2` = MamaDuck  
    - `3` = PapaDuck
    - `4` = DetectorDuck
  - **DeviceID**: Source device identifier (SDUID) - 8 character string
  - **Message**: Actual payload data from the source device

This format matches the official ClusterDuck Protocol examples and is compatible with the web dashboard in `examples/Basic-Ducks/PapaDuck/web/`.

## Example Configurations

### 1. Test Broker (No TLS, No Auth)

Matches the official PapaDuck example configuration:

```json
{
  "mqtt_conf": {
    "enabled": true,
    "server": "test.mosquitto.org",
    "port": 1883,
    "client_id": "papa-duck-mqtt-1",
    "pub_topic": "hub/event",
    "sub_topic": "incoming/say_hello",
    "keepalive": 60
  }
}
```

### 2. Test Broker with TLS (Port 8883)

```json
{
  "mqtt_conf": {
    "enabled": true,
    "server": "test.mosquitto.org",
    "port": 8883,
    "client_id": "papa-duck-mqtt-1",
    "pub_topic": "hub/event",
    "sub_topic": "incoming/say_hello",
    "keepalive": 60,
    "use_tls": true,
    "ca_cert_file": "/etc/ssl/certs/ca-certificates.crt"
  }
}
```

**Mosquitto Test Broker CA Certificate**: Available at https://test.mosquitto.org/ssl/mosquitto.org.crt

### 3. Private Broker with Authentication

```json
{
  "mqtt_conf": {
    "enabled": true,
    "server": "mqtt.example.com",
    "port": 8883,
    "client_id": "papa-duck-prod-001",
    "username": "gateway_user",
    "password": "secure_password",
    "pub_topic": "hub/event",
    "sub_topic": "incoming/say_hello",
    "keepalive": 60,
    "use_tls": true,
    "ca_cert_file": "/etc/ssl/certs/ca-certificates.crt"
  }
}
```

### 4. AWS IoT Core

For AWS IoT Core integration, follow the AWS-PapaDuck example pattern:

```json
{
  "mqtt_conf": {
    "enabled": true,
    "server": "your-endpoint.iot.us-east-1.amazonaws.com",
    "port": 8883,
    "client_id": "PAPADUCK",
    "pub_topic": "owl/device/PAPADUCK/evt",
    "keepalive": 60,
    "use_tls": true,
    "ca_cert_file": "/etc/ssl/certs/AmazonRootCA1.pem"
  }
}
```

**Note**: AWS IoT Core requires client certificates for mutual TLS authentication. See the `examples/Custom-Papa-Examples/AWS-PapaDuck` example in the ClusterDuck Protocol repository for complete setup.

### 5. Local MQTT Broker

```json
{
  "mqtt_conf": {
    "enabled": true,
    "server": "localhost",
    "port": 1883,
    "client_id": "local-gateway",
    "pub_topic": "hub/event"
  }
}
```

## Current Implementation Status

### ✅ Implemented
- MQTT configuration parsing from `global_conf.json`
- Message formatting and logging
- Configuration validation
- JSON message serialization

## MQTT Library Integration

This gateway uses the **Eclipse Paho MQTT C** library, which is already integrated and configured.

### Installation (Debian/Ubuntu)

```bash
sudo apt-get install libpaho-mqtt-dev libpaho-mqtt3c
```

### Build

The Makefile is already configured to link against Paho MQTT:

```bash
cd /home/zaihan/Projects/sx1302_hal/clusterduck
make
```

## Testing

### Subscribe to Messages (mosquitto_sub)

Following the PapaDuck pattern, subscribe to the `hub/event` topic:

```bash
# Basic subscription (no TLS)
mosquitto_sub -h test.mosquitto.org -t "hub/event" -v

# With TLS
mosquitto_sub -h test.mosquitto.org -p 8883 \
  --cafile /etc/ssl/certs/ca-certificates.crt \
  -t "hub/event" -v

# With authentication
mosquitto_sub -h mqtt.example.com -p 8883 \
  --cafile /etc/ssl/certs/ca-certificates.crt \
  -u "gateway_user" -P "secure_password" \
  -t "hub/event" -v
```

### Web Dashboard

The ClusterDuck Protocol includes a web dashboard for visualizing MQTT messages. See `examples/Basic-Ducks/PapaDuck/web/` for the HTML/JavaScript dashboard that connects to your MQTT broker and displays CDP messages in real-time.

**Dashboard Configuration** (in `web/app.js`):
```javascript
const mqttBrokerUrl = "wss://test.mosquitto.org:8081";  // WebSocket URL
client.subscribe("hub/event");  // Subscribe to PapaDuck topic
```

### Verify Logs

When running `clusterduckd`, you should see logs matching the PapaDuck pattern:

```
INFO: MQTT is enabled
INFO: MQTT server is configured to "test.mosquitto.org"
INFO: MQTT port is configured to 1883
INFO: MQTT publish topic is configured to "hub/event"
INFO: MQTT subscribe topic is configured to "incoming/say_hello"
...
[HUB] MQTT client connecting to broker...
[HUB] MQTT client connected
[HUB] got packet
[HUB] got topic: status from MAMADUCK
[HUB] Publish ok
```

### Example Message Output

When a MamaDuck sends a message, you'll see JSON output like:

```json
{
  "from": "hub",
  "to": "controller",
  "RE": false,
  "eventType": "status",
  "MessageID": "A3F9",
  "payload": {
    "hops": 1,
    "duckType": 2,
    "DeviceID": "MAMADUCK",
    "Message": "C:42|FM:123456"
  }
}
```

## TLS/SSL Configuration

### Obtaining CA Certificates

#### Test Broker (test.mosquitto.org)

For the Mosquitto test broker, download the CA certificate:

```bash
# Download Mosquitto test broker CA cert
wget https://test.mosquitto.org/ssl/mosquitto.org.crt -O /etc/ssl/certs/mosquitto.org.crt

# Or use system CA bundle (Debian/Ubuntu)
# /etc/ssl/certs/ca-certificates.crt
```

#### Cloud Providers

**AWS IoT Core:**
```bash
wget https://www.amazontrust.com/repository/AmazonRootCA1.pem -O /etc/ssl/certs/AmazonRootCA1.pem
```

**System CA Certificates:**
- Debian/Ubuntu: `/etc/ssl/certs/ca-certificates.crt`
- RHEL/CentOS: `/etc/pki/tls/certs/ca-bundle.crt`
- Alpine: `/etc/ssl/cert.pem`

### TLS Configuration Example

Following the PapaDuck pattern with embedded CA certificate:

```c
// In your application code (if needed)
const char* ca_cert = 
"-----BEGIN CERTIFICATE-----\n"
"MIIEAzCCAuugAwIBAgIUBY1hlCGvdj4NhBXkZ/uLUZNILAwwDQYJKoZIhvcNAQEL\n"
// ... rest of certificate ...
"-----END CERTIFICATE-----\n";

wifiClient.setCACert(ca_cert);
```

Or use a file path in `global_conf.json`:

```json
{
  "mqtt_conf": {
    "use_tls": true,
    "ca_cert_file": "/etc/ssl/certs/ca-certificates.crt"
  }
}
```

## Troubleshooting

### Common Issues

1. **MQTT not enabled**: 
   - Verify `"enabled": true` in `global_conf.json`
   - Check MQTT library is installed: `ldconfig -p | grep paho-mqtt`

2. **Connection refused**: 
   - Verify broker hostname and port
   - Test connection: `telnet test.mosquitto.org 1883`
   - For TLS: `openssl s_client -connect test.mosquitto.org:8883`

3. **Authentication failed**: 
   - Double-check `username` and `password` fields
   - Verify broker requires authentication

4. **TLS errors**: 
   - Verify `ca_cert_file` path exists and is readable
   - Check certificate matches broker
   - Ensure `use_tls: true` and `port: 8883`
   - Server name must match certificate CN/SAN

5. **Messages not appearing**: 
   - Subscribe to `hub/event` topic
   - Check broker logs
   - Use wildcard: `mosquitto_sub -t "hub/#"`

### Debug Commands

```bash
# Test MQTT connection (plain)
mosquitto_pub -h test.mosquitto.org -t "test" -m "hello"

# Test with TLS
mosquitto_pub -h test.mosquitto.org -p 8883 \
  --cafile /etc/ssl/certs/ca-certificates.crt \
  -t "test" -m "hello" -d

# Verify TLS connection
openssl s_client -connect test.mosquitto.org:8883 \
  -CAfile /etc/ssl/certs/ca-certificates.crt

# Check certificate details
openssl x509 -in /etc/ssl/certs/ca-certificates.crt -text -noout
```

## Implementation Notes

This gateway implementation follows the **ClusterDuck Protocol PapaDuck** pattern for MQTT integration:

### Architecture
- **Role**: PapaDuck (Gateway/Hub)
- **Function**: Receives packets from mesh network via SX1302 HAL and forwards to MQTT broker
- **Message Format**: Standard CDP JSON format (compatible with web dashboard)
- **Topics**: 
  - Publish: `hub/event` (all CDP messages)
  - Subscribe: `incoming/say_hello` (downlink commands)

### Compatibility
- ✅ Compatible with official ClusterDuck Protocol PapaDuck examples
- ✅ Works with the CDP web dashboard (`examples/Basic-Ducks/PapaDuck/web/`)
- ✅ Supports same MQTT brokers (test.mosquitto.org, AWS IoT Core, etc.)
- ✅ Uses identical JSON message format
- ✅ Follows same topic naming conventions

### Current Status
- ✅ MQTT configuration parsing from `global_conf.json`
- ✅ TLS/SSL support with CA certificate validation
- ✅ Username/password authentication
- ✅ Message formatting (CDP JSON standard)
- ✅ Eclipse Paho MQTT C client integrated
- ✅ Automatic reconnection handling
- ⚠️ Downlink command processing (subscribe topic) - planned

## References

### ClusterDuck Protocol
- [ClusterDuck Protocol Repository](https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol)
- [PapaDuck MQTT Example](https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/tree/main/examples/Basic-Ducks/PapaDuck)
- [AWS PapaDuck Example](https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/tree/main/examples/Custom-Papa-Examples/AWS-PapaDuck)
- [Web Dashboard](https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol/tree/main/examples/Basic-Ducks/PapaDuck/web)

### MQTT Resources
- [Eclipse Paho MQTT C](https://github.com/eclipse/paho.mqtt.c)
- [MQTT.org Protocol Specification](https://mqtt.org/)
- [HiveMQ MQTT Essentials](https://www.hivemq.com/mqtt-essentials/)
- [Mosquitto Test Broker](https://test.mosquitto.org/)

### TLS/SSL
- [Let's Encrypt](https://letsencrypt.org/)
- [AWS IoT Core Security](https://docs.aws.amazon.com/iot/latest/developerguide/security.html)
- [OpenSSL Documentation](https://www.openssl.org/docs/)
