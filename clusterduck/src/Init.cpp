#include <CDP.h>
#include <queue>

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

    // Forward the counter message to the MQTT broker
    // This is a simple example, but you can do anything you want with the message here
    // This example shows how the message from be transformed into something that matches your application
    // uint32_t msgId = esp_random();
    doc["from"] = "hub";
    doc["to"] = "controller";
    doc["RE"] = false; // This flag is used to indicate if the controller should respond to this message
    doc["eventType"] = cdpTopic.c_str();
    doc["MessageID"].set(muid);
    doc["payload"]["hops"].set(cdp_packet.hopCount);
    doc["payload"]["duckType"].set(cdp_packet.duckType);
    doc["payload"]["DeviceID"] = sduid.c_str();

    doc["payload"]["Message"] = payload.c_str();

    std::string jsonstat;
    serializeJson(doc, jsonstat);
    printf("%s\n",jsonstat.c_str());
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
    int err = hub.begin();
    if (err != DUCK_ERR_NONE) {
        printf("[HUB] ERROR: Failed to setup LoRa radio, err=%d\n", err);
        return nullptr;
    }
    
    // Set the receive callback
    hub.onReceiveDuckData(handleDuckData);
    
    printf("[HUB] ClusterDuck Protocol initialized successfully\n");
    return (void*)&hub;
}
