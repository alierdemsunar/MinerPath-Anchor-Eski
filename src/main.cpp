#include <Arduino.h>
#include <SPI.h>
#include <DW1000.h>
#include <EthernetUDP.h>
#include <Dns.h>
#include <ArduinoJson.h>

const boolean DEVELOPMENT_MODE = true; // Development mode switch

const uint8_t DWM1000_RESET_PIN = 3; // DWM1000 reset PIN
const uint8_t DWM1000_IRQ_PIN = 2; // DWM1000 IRQ PIN
const uint8_t DWM1000_SELECT_PIN = 48; // DWM1000 select PIN
IPAddress TERMINAL_IP(0, 0, 0, 0); // Terminal IP address
unsigned int TERMINAL_PORT = 5000; // Terminal Port
unsigned int LOCAL_PORT = 5000; // Local Port
EthernetUDP UDP_PACKET;

String RECEIVED_MESSAGE;

//***UNIQUE PARAMETERS***//
char TERMINAL_DOMAIN[32] = "terminal.minerpath.local"; // Terminal domain address
byte ANCHOR_MAC_ADDRESS[6] = {0x90, 0x92, 0xBE, 0xEF, 0xFE, 0xEF}; // Anchor MAC address
char ANCHOR_UNIQUE_ID[24] = "67:92:5B:D5:A9:9A:E2:9F"; // Anchor unique ID
unsigned int ANCHOR_DEVICE_ADDRESS = 6;
unsigned int ANCHOR_NETWORK_ID = 10;

volatile boolean ANCHOR_RECEIVED = false;
volatile boolean ANCHOR_ERROR = false;

void minerPathAnchorReceived() {
  ANCHOR_RECEIVED = true;
}

void minerPathAnchorError() {
  ANCHOR_ERROR = true;
}

void minerPathAnchorSetup() {
  DW1000.begin(DWM1000_IRQ_PIN, DWM1000_RESET_PIN);
  DW1000.select(DWM1000_SELECT_PIN);
  Serial.println(F("[✓] Anchor module initialized."));
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(ANCHOR_DEVICE_ADDRESS);
  DW1000.setNetworkId(ANCHOR_NETWORK_ID);
  DW1000.setEUI(ANCHOR_UNIQUE_ID);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.commitConfiguration();
  Serial.println(F("[✓] Anchor module configured successfully."));

  DW1000.attachReceivedHandler(minerPathAnchorReceived);
  DW1000.attachReceiveFailedHandler(minerPathAnchorError);
  DW1000.attachErrorHandler(minerPathAnchorError);

  DW1000.newReceive();
  DW1000.setDefaults();
  // so we don't need to restart the receiver manually
  DW1000.receivePermanently(true);
  DW1000.startReceive();

  //***DEVELOPMENT ONLY CODE START***//
  if(DEVELOPMENT_MODE) {
    char info[24];
    DW1000.getPrintableExtendedUniqueIdentifier(info);
    Serial.print("[!] Anchor Unique ID: "); Serial.println(info);
  }
  //***DEVELOPMENT ONLY CODE END***//

  /* RANGING */

}

void terminalDNSLookup(const char* DOMAIN, IPAddress IP) {
  DNSClient DNS_CLIENT;
  DNS_CLIENT.begin(Ethernet.dnsServerIP());
  DNS_CLIENT.getHostByName(DOMAIN, IP);
  //***DEVELOPMENT ONLY CODE START***//
  if(DEVELOPMENT_MODE) {
    Serial.println("[?] Trying to find terminal IP via DNS...");
    if(DNS_CLIENT.getHostByName(DOMAIN, IP) == 1) {
      Serial.print("[!] Terminal IP: ");Serial.print(IP);Serial.print(" (");Serial.print(DOMAIN);Serial.println(")");
    } else {
      Serial.print("[X] IP of \"");Serial.print(DOMAIN);Serial.println("\" terminal was not found via DNS!");
    }
  }
  //***DEVELOPMENT ONLY CODE END***//
  if(DNS_CLIENT.getHostByName(DOMAIN, IP) == 1) {
    TERMINAL_IP = IP;
  }
}

void setup() {
  //***DEVELOPMENT ONLY CODE START***//
  if(DEVELOPMENT_MODE) {
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("[?] Trying to activate ethernet module...");
    if (Ethernet.begin(ANCHOR_MAC_ADDRESS) == 0) {
        Serial.println("[X] Failed to configure ethernet MAC address!");
      if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("[X] Ethernet module was not found!");
      } else if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("[X] Ethernet cable is not connected!");
      }
      while (true) {
        delay(1);
      }
    } else {
      Serial.print("[!] MinerPath-Anchor MAC: ");Serial.print(ANCHOR_MAC_ADDRESS[0], HEX);Serial.print(":");Serial.print(ANCHOR_MAC_ADDRESS[1], HEX);Serial.print(":");Serial.print(ANCHOR_MAC_ADDRESS[2], HEX);Serial.print(":");Serial.print(ANCHOR_MAC_ADDRESS[3], HEX);Serial.print(":");Serial.print(ANCHOR_MAC_ADDRESS[4], HEX);Serial.print(":");Serial.println(ANCHOR_MAC_ADDRESS[5], HEX);
      Serial.print("[!] MinerPath-Anchor IP: ");Serial.print(Ethernet.localIP());Serial.print("/");Serial.print(Ethernet.subnetMask());Serial.print("/");Serial.println(Ethernet.gatewayIP());
      Serial.print("[!] MinerPath-Anchor DNS: ");Serial.println(Ethernet.dnsServerIP());
      Serial.println("[✓] Ethernet module activated.");
    }
  }
  //***DEVELOPMENT ONLY CODE END***//
  UDP_PACKET.begin(LOCAL_PORT);
  terminalDNSLookup(TERMINAL_DOMAIN, TERMINAL_IP);
  minerPathAnchorSetup();
}

void loop() {
  
  if (ANCHOR_RECEIVED) {
    // get data as string
    DW1000.getData(RECEIVED_MESSAGE);
    Serial.print("Data is ... "); Serial.println(RECEIVED_MESSAGE);
    Serial.print("FP power is [dBm] ... "); Serial.println(DW1000.getFirstPathPower());
    Serial.print("RX power is [dBm] ... "); Serial.println(DW1000.getReceivePower());
    Serial.print("Signal quality is ... "); Serial.println(DW1000.getReceiveQuality());
    ANCHOR_RECEIVED = false;
  }
  if (ANCHOR_ERROR) {
    Serial.println("Error receiving a message");
    ANCHOR_ERROR = false;
    DW1000.getData(RECEIVED_MESSAGE);
    Serial.print("Error data is ... "); Serial.println(RECEIVED_MESSAGE);
  }
  



  /*
  UDP_PACKET.beginPacket(TERMINAL_IP, TERMINAL_PORT);
  StaticJsonDocument <256> doc;
  doc["anchor_id"] = ANCHOR_UNIQUE_ID;
  doc["type"] = "anchor";
  serializeJson(doc, UDP_PACKET);
  UDP_PACKET.endPacket();
  delay(1000);
  */
}
