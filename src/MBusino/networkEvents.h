#if defined(ESP32)
// --- WiFi diagnostics (scratch build test/wifi-diag) ---
static const char* wifiReasonName(uint8_t r){
  switch(r){
    case 2:   return "AUTH_EXPIRE";
    case 15:  return "4WAY_HANDSHAKE_TIMEOUT";
    case 200: return "BEACON_TIMEOUT";
    case 201: return "NO_AP_FOUND";
    case 202: return "AUTH_FAIL";
    case 203: return "ASSOC_FAIL";
    case 204: return "HANDSHAKE_TIMEOUT";
    case 205: return "CONNECTION_FAIL";
    case 210: return "NO_AP_FOUND_W_COMPATIBLE_SECURITY";
    case 211: return "NO_AP_FOUND_IN_AUTHMODE_THRESHOLD";
    case 212: return "NO_AP_FOUND_IN_RSSI_THRESHOLD";
    default:  return "unknown";
  }
}

// Scratch-build diagnostics: prints STA connect/disconnect details (reason codes etc.).
void WiFiDiagEvent(arduino_event_t *event){
  if(event->event_id == ARDUINO_EVENT_WIFI_STA_DISCONNECTED){
    uint8_t r = event->event_info.wifi_sta_disconnected.reason;
    Serial.printf("[WIFI-DIAG] disconnect reason=%u (%s) ssid=%.*s status=%d rssi=%d\n",
                  r, wifiReasonName(r),
                  (int)event->event_info.wifi_sta_disconnected.ssid_len,
                  (const char*)event->event_info.wifi_sta_disconnected.ssid,
                  (int)WiFi.status(), (int)WiFi.RSSI());
  }
  else if(event->event_id == ARDUINO_EVENT_WIFI_STA_CONNECTED){
    Serial.printf("[WIFI-DIAG] connected: rssi=%d channel=%d bssid=%02X:%02X:%02X:%02X:%02X:%02X\n",
                  (int)WiFi.RSSI(), (int)WiFi.channel(),
                  event->event_info.wifi_sta_connected.bssid[0],
                  event->event_info.wifi_sta_connected.bssid[1],
                  event->event_info.wifi_sta_connected.bssid[2],
                  event->event_info.wifi_sta_connected.bssid[3],
                  event->event_info.wifi_sta_connected.bssid[4],
                  event->event_info.wifi_sta_connected.bssid[5]);
  }
  else if(event->event_id == ARDUINO_EVENT_WIFI_STA_GOT_IP){
    Serial.printf("[WIFI-DIAG] got IP %s rssi=%d channel=%d\n",
                  WiFi.localIP().toString().c_str(), (int)WiFi.RSSI(), (int)WiFi.channel());
  }
}
#endif // ESP32

// WARNING: This function is called from a separate FreeRTOS task (thread)!
void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);

  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:               Serial.println("WiFi interface ready"); break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:           Serial.println("Completed scan for access points"); break;
    case ARDUINO_EVENT_WIFI_STA_START:           Serial.println("WiFi client started"); break;
    case ARDUINO_EVENT_WIFI_STA_STOP:            Serial.println("WiFi clients stopped"); break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("Connected to access point"); 
      pulseInterval = 500;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:    
      Serial.println("Disconnected from WiFi access point"); 
      wifiReconnect = true;
      timerWifiReconnect = millis();
      pulseInterval = 1000;
    break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: Serial.println("Authentication mode of access point has changed"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:        
      Serial.println("Lost IP address and IP address is reset to 0"); 
      Serial.println("MBusino will restart soon"); 
      pulseInterval = 10;
    timerReboot = millis();
    waitForRestart = true;
    break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:          Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_FAILED:           Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:          Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode"); break;
    case ARDUINO_EVENT_WPS_ER_PIN:              Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode"); break;
    case ARDUINO_EVENT_WIFI_AP_START:           Serial.println("WiFi access point started"); break;
    case ARDUINO_EVENT_WIFI_AP_STOP:            Serial.println("WiFi access point  stopped"); break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:    Serial.println("Client connected"); break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: Serial.println("Client disconnected"); break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:   Serial.println("Assigned IP address to client"); break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:  Serial.println("Received probe request"); break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:         Serial.println("AP IPv6 is preferred"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:        Serial.println("STA IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_GOT_IP6:             Serial.println("Ethernet IPv6 is preferred"); break;
    case ARDUINO_EVENT_ETH_START:               Serial.println("Ethernet started"); break;
    case ARDUINO_EVENT_ETH_STOP:                Serial.println("Ethernet stopped"); break;
    case ARDUINO_EVENT_ETH_CONNECTED:           Serial.println("Ethernet connected"); break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:        Serial.println("Ethernet disconnected"); break;
    case ARDUINO_EVENT_ETH_GOT_IP:              Serial.println("Obtained IP address"); break;
    default:                                    break;
  }
}