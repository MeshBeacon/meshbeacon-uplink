# MQTT Configuration for ClusterDuck "SuperDuck" Gateway

## Overview

The ClusterDuck gateway (`clusterduckd`) now supports publishing received packets to an MQTT broker. This allows integration with cloud services, IoT platforms, and other MQTT-enabled applications.

## Configuration

MQTT settings are configured in the `global_conf.json` file under the `mqtt_conf` object.

### Example Configuration

```json
{
  "mqtt_conf": {
    "enabled": true,
    "server": "test.mosquitto.org",
    "port": 1883,
    "client_id": "papa-duck-gateway-1",
    "username": "",
    "password": "",
    "pub_topic": "hub/event",
    "sub_topic": "incoming/say_hello",
    "keepalive": 60,
    "use_tls": false,
    "ca_cert_file": ""
  }
}
```

### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `enabled` | boolean | `false` | Enable/disable MQTT publishing |
| `server` | string | `""` | MQTT broker hostname or IP address |
| `port` | integer | `1883` | MQTT broker port (1883 for plain, 8883 for TLS) |
| `client_id` | string | `"papa-duck-gateway-1"` | Unique MQTT client identifier |
| `username` | string | `""` | MQTT authentication username (optional) |
| `password` | string | `""` | MQTT authentication password (optional) |
| `pub_topic` | string | `"hub/event"` | Topic to publish ClusterDuck messages |
| `sub_topic` | string | `"incoming/say_hello"` | Topic to subscribe for downlink commands |
| `keepalive` | integer | `60` | MQTT keepalive interval in seconds |
| `use_tls` | boolean | `false` | Enable TLS/SSL encryption |
| `ca_cert_file` | string | `""` | Path to CA certificate file for TLS |

## Message Format

Published messages follow the same JSON format as the ClusterDuck Protocol:

```json
{
  "from": "hub",
  "to": "controller",
  "RE": false,
  "eventType": "health",
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

- **from**: Always "hub" (the gateway)
- **to**: Target recipient (typically "controller")
- **RE**: Response expected flag
- **eventType**: CDP topic (health, ping, pong, cmd, etc.)
- **MessageID**: Unique message identifier (MUID)
- **payload**:
  - **hops**: Number of hops the packet traveled
  - **duckType**: Duck type (1=DuckLink, 2=MamaDuck, 3=PapaDuck, etc.)
  - **DeviceID**: Source device identifier (SDUID)
  - **Message**: Actual payload data

## Example Configurations

### 1. Public Test Broker (No Auth)

```json
{
  "mqtt_conf": {
    "enabled": true,
    "server": "test.mosquitto.org",
    "port": 1883,
    "client_id": "papa-duck-gateway-1",
    "pub_topic": "clusterduck/gateway/events"
  }
}
```

### 2. Secure Broker with TLS

```json
{
  "mqtt_conf": {
    "enabled": true,
    "server": "mqtt.example.com",
    "port": 8883,
    "client_id": "papa-duck-gateway-prod",
    "username": "gateway_user",
    "password": "secure_password",
    "pub_topic": "iot/clusterduck/events",
    "keepalive": 60,
    "use_tls": true,
    "ca_cert_file": "/etc/ssl/certs/ca-certificates.crt"
  }
}
```

### 3. Local MQTT Broker

```json
{
  "mqtt_conf": {
    "enabled": true,
    "server": "localhost",
    "port": 1883,
    "client_id": "local-gateway",
    "pub_topic": "home/clusterduck/events"
  }
}
```

## Current Implementation Status

### ✅ Implemented
- MQTT configuration parsing from `global_conf.json`
- Message formatting and logging
- Configuration validation
- JSON message serialization

## Integrating MQTT Library

To enable actual MQTT publishing, you need to integrate an MQTT client library. Recommended: **Eclipse Paho MQTT C**.

### Installation (Debian/Ubuntu)

```bash
sudo apt-get install libpaho-mqtt-dev
```

### Build

```
make
```

## Testing

### Subscribe to Messages

```bash
# Using mosquitto_sub
mosquitto_sub -h test.mosquitto.org -t "hub/event" -v

# With TLS
mosquitto_sub -h mqtt.example.com -p 8883 \
  --cafile /etc/ssl/certs/ca-certificates.crt \
  -t "iot/clusterduck/events" -v
```

### Verify Logs

When running `clusterduckd`, you should see:

```
INFO: MQTT is enabled
INFO: MQTT server is configured to "test.mosquitto.org"
INFO: MQTT port is configured to 1883
INFO: MQTT publish topic is configured to "hub/event"
...
[MQTT] Publishing to topic: hub/event
[MQTT] Message (152 bytes): {"from":"hub","to":"controller",...}
```

## Troubleshooting

1. **MQTT not working**: Check `enabled: true` in config
2. **Connection refused**: Verify broker hostname and port
3. **Authentication failed**: Double-check username/password
4. **TLS errors**: Ensure `ca_cert_file` path is correct
5. **Messages not appearing**: Subscribe to correct topic with wildcards (`#` or `+`)

## References

- [Eclipse Paho MQTT C](https://github.com/eclipse/paho.mqtt.c)
- [MQTT.org](https://mqtt.org/)
- [HiveMQ MQTT Essentials](https://www.hivemq.com/mqtt-essentials/)
- [ClusterDuck Protocol](https://github.com/ClusterDuck-Protocol/ClusterDuck-Protocol)
