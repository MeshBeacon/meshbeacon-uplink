#ifndef PAPADUCK_H
#define PAPADUCK_H

#include "Duck.h"
#include "../CdpPacket.h"
#include "../bridge/ClusterDuckBridge.h"  // Unified bridge C++ API

template <typename RadioType = DuckLoRa>
class PapaDuck : public Duck<RadioType> {
public:
  using Duck<RadioType>::Duck;
  
  PapaDuck(std::string name = "PAPADUCK") : Duck<RadioType>(std::move(name)) {}
  ~PapaDuck() {}

  /// Papa Duck callback functions signature.
  using rxDoneCallback = void (*)(CdpPacket data);
  using txDoneCallback = void (*)(void);
  /**
   * @brief Register callback for handling data received from duck devices
   * 
   * The callback will be invoked if the packet needs to be relayed (i.e not seen before)
   * @param cb a callback to handle data received by the papa duck
   */
  void onReceiveDuckData(rxDoneCallback cb) { this->recvDataCallback = cb; }

  /**
   * @brief Get the DuckType
   *
   * @returns the duck type defined as DuckType
   */
  DuckType getType() { return DuckType::PAPA; }

  /**
   * @brief Public wrapper to initialize the LoRa radio
   * This allows external C code to initialize the radio setup
   * @returns DUCK_ERR_NONE if successful, error code otherwise
   */
  int begin(const LoRaConfigParams& config = RadioType::defaultRadioParams) {
    printf("[PAPADUCK] begin() called, about to call setupLoRaRadio()\n");
    int err = this->setupLoRaRadio(config);
    printf("[PAPADUCK] setupLoRaRadio() returned: %d\n", err);
    if (err == DUCK_ERR_NONE) {
      // PapaDuck is always PUBLIC (gateway mode), skip network search
      this->router.setNetworkState(NetworkState::PUBLIC);
      loginfo_ln("[PAPADUCK] Network state set to PUBLIC (gateway mode)");
    } else {
      printf("[PAPADUCK] ERROR: setupLoRaRadio() failed with error: %d\n", err);
    }
    return err;
  }

  /**
   * @brief Process any received packets
   * This is a simplified version of main() that skips network joining logic
   */
  void processPackets() {
    // Check if we have packets from the forwarder bridge (SX1302 HAL) using unified API
    bool has_bridge_packet = cdp_bridge::has_uplink_packet();
    
    if (this->duckRadio.getReceiveFlag() || has_bridge_packet) {
      if (has_bridge_packet) {
        printf("[PAPADUCK] Bridge has packet, calling handleReceivedPacket()\n");
      } else {
        printf("[PAPADUCK] Receive flag is set, calling handleReceivedPacket()\n");
      }
      this->handleReceivedPacket();
    }
  }

  //remove this when mqtt quack pack is added
  bool isWifiConnected(){
    return this->duckWifi.connected();
  }

private:
  rxDoneCallback recvDataCallback;

  void handleReceivedPacket() {
    loginfo_ln("====> handleReceivedPacket: START");

    int err;
    std::optional<std::vector<uint8_t>> rxData = this->duckRadio.readReceivedData();
    if (!rxData) {
    logerr_ln("ERROR failed to get data from DuckRadio.");
    return;
    }
    CdpPacket rxPacket(rxData.value());
    logdbg_ln("Got data from radio. size: %d",rxPacket.size());

    // Check bloom filter BEFORE processing to prevent duplicate callbacks
    bool alreadySeen = this->router.getFilter().bloom_check(rxPacket.muid.data(), MUID_LENGTH);
    if (alreadySeen) {
        loginfo_ln("====> Duplicate packet detected (MUID already seen), ignoring");
        return;
    }

    // Add to bloom filter immediately to mark as seen
    this->router.getFilter().bloom_add(rxPacket.muid.data(), MUID_LENGTH);

    // Now invoke callback and process packet
    recvDataCallback(rxPacket);
    
    printf("[PAPADUCK] Callback completed, continuing packet processing\n");

    //Check if Duck is desitination for this packet before relaying
    if (duckutils::isEqual(BROADCAST_DUID, rxPacket.dduid)) {
        ifBroadcast(rxPacket, err);
    } else if(duckutils::isEqual(this->duid, rxPacket.dduid) || duckutils::isEqual(rxPacket.dduid, PAPADUCK_DUID)) { //Target device check
        ifNotBroadcast(rxPacket, err);
    } else { //If it's meant for a specific target but not this one
        ifNotBroadcast(rxPacket, err, true);
    }
    
    printf("[PAPADUCK] handleReceivedPacket completed\n");
  } 

  void ifBroadcast(CdpPacket rxPacket, int err) { 
    printf("[PAPADUCK] ifBroadcast START, topic=%d\n", (int)rxPacket.topic);
    switch(rxPacket.topic) {
        case reservedTopic::rreq: {
            printf("[PAPADUCK] Processing RREQ, hopCount=%d\n", rxPacket.hopCount);
            if(rxPacket.hopCount <= 0){
                std::string sduidStr = duckutils::toString(rxPacket.sduid);
                loginfo_ln("RREQ received from %s. Sending Response!", sduidStr.c_str());
                printf("[PAPADUCK] Creating RouteJSON for RREP...\n");
                RouteJSON rrepDoc = RouteJSON(rxPacket.sduid, this->duid);
                printf("[PAPADUCK] Adding to path...\n");
                rrepDoc.addToPath(this->duid);
                printf("[PAPADUCK] Sending route response...\n");
                this->sendRouteResponse(rxPacket.sduid, rrepDoc.asString());
                // Update routing table with signal info
                this->router.insertIntoRoutingTable(rxPacket.sduid, rxPacket.sduid, this->getSignalScore()); //can only be one hop away
                printf("[PAPADUCK] RREQ processing completed\n");
            }
            break;
        }
        case reservedTopic::ping:
            loginfo_ln("PING received. Sending PONG!");
            err = this->sendPong();
            if (err != DUCK_ERR_NONE) {
                logerr_ln("ERROR failed to send pong message. rc = %d",err);
            }
            break;
        case reservedTopic::pong:
            loginfo_ln("PONG received. Ignoring!");
            break;
        // case reservedTopic::cmd:
        //     loginfo_ln("Command received");

        //     err = this->broadcastPacket(rxPacket);
            
        //     if (err != DUCK_ERR_NONE) {
        //         logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
        //     } else {
        //         loginfo_ln("handleReceivedPacket: packet RELAY DONE");
        //     }
        //     break;
        default:
            err = this->broadcastPacket(rxPacket);
            if (err != DUCK_ERR_NONE) {
                logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
            } else {
                loginfo_ln("handleReceivedPacket: packet RELAY DONE");
            }
    }
}

void ifNotBroadcast(CdpPacket rxPacket, int err, bool relay = false) {
    switch(rxPacket.topic) {
        case reservedTopic::rreq: {
            RouteJSON rreqDoc = RouteJSON(rxPacket.data);
            //route requests are just forwarded so we can use the sduid as the origin
            std::optional<Duid> last = rreqDoc.getlastInPath();
            Duid lastInPath = last.has_value() ? last.value() : rxPacket.sduid;
            if(!relay) {
                loginfo_ln("handleReceivedPacket: Sending RREP");
                rxPacket.data = duckutils::stringToByteVector(rreqDoc.convertReqToRep());
                this->sendRouteResponse(lastInPath, rreqDoc.asString());
            } else {
                rxPacket.data = duckutils::stringToByteVector(rreqDoc.addToPath(this->duid)); //why is this different from stringToArray
                err = this->forwardPacket(rxPacket);
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay RREQ. rc = %d",err);
                } else {
                    loginfo_ln("handleReceivedPacket: RREQ packet RELAY DONE");
                }
            }
        }
        break;
      
        case reservedTopic::rrep: {
            //we still need to recieve rreps in case of ttl expiry
            RouteJSON rrepDoc = RouteJSON(rxPacket.data);
            std::optional<Duid> last = rrepDoc.getlastInPath();
            Duid lastInPath = last.has_value() ? last.value() : rxPacket.sduid;
            std::string sduidStr = duckutils::toString(rxPacket.sduid);
            loginfo_ln("Received Route Response from DUID: %s", sduidStr.c_str());

            std::optional<Duid> nextHop = this->router.getBestNextHop(rrepDoc.getDestination());
            if((rrepDoc.getDestination() != this->duid) && (nextHop.has_value()) && (nextHop.value() !=  rxPacket.sduid)){
                rrepDoc.popFromPath();
                rrepDoc.addToPath(this->duid);
                //route responses need a way to keep tray of who relayed the packet, but a response needs to be directed and not broadly relayed
                this->sendRouteResponse(rrepDoc.getDestination(), rrepDoc.asString()); //so here the "relaying" duck is known from sduid
                this->router.insertIntoRoutingTable(rxPacket.sduid, lastInPath, this->getSignalScore());
            } else {
                //destination = sender of the rrep -> the last hop to current duck
                this->router.insertIntoRoutingTable(rrepDoc.getOrigin(), lastInPath, this->getSignalScore());
            }
        }
            break;
        case reservedTopic::ping:
            loginfo_ln("PING received. Sending PONG!");
            err = this->sendPong();
            if (err != DUCK_ERR_NONE) {
                logerr_ln("ERROR failed to send pong message. rc = %d",err);
            }
            break;
        case reservedTopic::pong:
            loginfo_ln("PONG received. Ignoring!");
            break;
        // case reservedTopic::cmd:
        //     loginfo_ln("Command received");

        //     err = this->broadcastPacket(rxPacket);
            
        //     if (err != DUCK_ERR_NONE) {
        //         logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
        //     } else {
        //         loginfo_ln("handleReceivedPacket: packet RELAY DONE");
        //     }
        //     break;
        default:
          if(relay){
            this->forwardPacket(rxPacket);
          }               
    }
  }

};

#endif
