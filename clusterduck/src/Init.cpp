#include <CDP.h>
#include <queue>
#include "routing/RouteJSON.h"

// Forward declare C functions for MQTT publishing
extern "C" {
    void mqtt_publish_message(const char* topic, const char* message, int length);
}

// --- Global Objects ---
PapaDuck hub("PAPADUCK");

void processMessageFromDucks(std::vector<std::byte> packetBuffer);
void handleDuckData(CdpPacket receivedPacket);

void processMessageFromDucks(CdpPacket cdp_packet) {

    JsonDocument doc;

    int messageLength = cdp_packet.data.size();

    printf("Packet data size=%d\n", messageLength);

    std::string muid(cdp_packet.muid.begin(), cdp_packet.muid.end());
    std::string sduid(cdp_packet.sduid.begin(), cdp_packet.sduid.end());
    std::string cdpTopic = cdp_packet.topicToString();

    printf("[HUB] got topic: %s from %s\n",cdpTopic.c_str(), sduid.c_str());

    // Counter Message
    std::string payload(cdp_packet.data.begin(), cdp_packet.data.end());

    // Build JSON message matching official PapaDuck format
    // Reference: ClusterDuck-Protocol/examples/Basic-Ducks/PapaDuck/PapaDuck.ino
    doc["from"] = "hub";
    doc["to"] = "controller";
    doc["RE"] = false;
    doc["eventType"] = cdpTopic.c_str();
    doc["MessageID"] = muid.c_str();
    
    // Payload fields at root level (official PapaDuck format)
    doc["payload"]["hops"] = cdp_packet.hopCount;
    doc["payload"]["duckType"] = cdp_packet.duckType;
    doc["payload"]["DeviceID"] = sduid.c_str();
    
    // For RREQ/RREP packets, extract and include the routing path
    // Don't include raw Message field since we're parsing it into structured data
    if (cdp_packet.topic == reservedTopic::rreq || cdp_packet.topic == reservedTopic::rrep) {
        printf("[HUB] Processing route packet, data size: %zu\n", cdp_packet.data.size());
        
        try {
            RouteJSON routeDoc(cdp_packet.data);
            
            // Extract origin and destination as strings
            Duid originDuid = routeDoc.getOrigin();
            Duid destDuid = routeDoc.getDestination();
            std::string originStr = duckutils::toString(originDuid);
            std::string destStr = duckutils::toString(destDuid);
            
            doc["payload"]["origin"] = originStr.c_str();
            doc["payload"]["destination"] = destStr.c_str();
            
            // Add path as JSON array
            JsonArray pathArray = doc["payload"]["path"].to<JsonArray>();
            
            const auto& pathVec = routeDoc.getPath();
            printf("[HUB] Parsed RouteJSON, path vector size: %zu\n", pathVec.size());
            
            if (pathVec.empty()) {
                // If path is empty, use the source device ID as fallback
                pathArray.add(sduid.c_str());
                printf("[HUB] Route packet has empty path, using DeviceID '%s' as fallback\n", sduid.c_str());
            } else {
                for (const auto& node : pathVec) {
                    pathArray.add(node);
                    printf("[HUB] Added path node: %s\n", node.c_str());
                }
                printf("[HUB] Route packet with path size: %zu\n", pathVec.size());
            }
        } catch (const std::exception& e) {
            printf("[HUB] ERROR: Exception parsing RouteJSON: %s, using DeviceID as fallback\n", e.what());
            doc["payload"]["origin"] = sduid.c_str();
            doc["payload"]["destination"] = "UNKNOWN";
            JsonArray pathArray = doc["payload"]["path"].to<JsonArray>();
            pathArray.add(sduid.c_str());
        }
    } else {
        // For non-routing packets, include the raw message
        doc["payload"]["Message"] = payload.c_str();
    }

    std::string jsonstat;
    serializeJson(doc, jsonstat);
    printf("%s\n",jsonstat.c_str());
    
    // Publish to MQTT if enabled
    // Note: mqtt_publish_message() copies the message internally (either to queue or MQTT lib)
    // so it's safe to pass a pointer to local string
    mqtt_publish_message("", jsonstat.c_str(), jsonstat.length());
    
    printf("[HUB] processMessageFromDucks completed\n");
}

// The callback method simply takes the incoming packet and
// converts it to a JSON string, before sending it out over MQTT
void handleDuckData(CdpPacket receivedPacket) {
  printf("[HUB] got packet\n");
  processMessageFromDucks(receivedPacket);
}

// C wrapper for initialization and setup
extern "C" void* hub_init_and_setup(void) {
    printf("[HUB] Initializing ClusterDuck Protocol...\n");
    
    // Set up the LoRa radio (this will register callbacks)
    printf("[HUB] Calling hub.begin()...\n");
    int err = hub.begin();
    printf("[HUB] hub.begin() returned: %d\n", err);
    
    if (err != DUCK_ERR_NONE) {
        printf("[HUB] ERROR: Failed to setup LoRa radio, err=%d\n", err);
        return nullptr;
    }
    
    // Set the receive callback
    printf("[HUB] Setting receive callback...\n");
    hub.onReceiveDuckData(handleDuckData);
    
    printf("[HUB] ClusterDuck Protocol initialized successfully\n");
    return (void*)&hub;
}

// C wrapper to call the hub's main processing loop
extern "C" void hub_run_loop(void) {
    // Call the hub's main() method which processes radio interrupts
    // and handles received packets
    hub.main();
}
