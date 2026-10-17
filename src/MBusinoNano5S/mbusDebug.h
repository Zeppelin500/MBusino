// M-Bus debug log for the web interface (/mbus): the last received telegrams and the last decoded telegram.
// WARNING: the web server runs in a separate FreeRTOS task, every access is guarded by mbusLogMutex!

#define MBUS_LOG_SIZE 10 // number of telegrams kept in the log

#define MBUS_LOG_OK 0
#define MBUS_LOG_NO_DATA 1
#define MBUS_LOG_BAD_FRAME 2

struct mbusLogEntry {
  uint32_t time;    // millis() when the entry was added
  uint8_t status;   // MBUS_LOG_...
  uint8_t address;  // requested M-Bus address
  uint16_t len;     // number of valid bytes in data
  uint8_t data[MBUS_DATA_SIZE];
};

mbusLogEntry mbusLog[MBUS_LOG_SIZE];
uint8_t mbusLogNext = 0;   // ring buffer position of the next entry
uint32_t mbusLogCount = 0; // number of entries since start
char mbusLastJson[sizeof(jsonstring)] = {0}; // last decoded telegram (header + records)
uint8_t mbusLastAddress = 0;
uint32_t mbusLastTime = 0;
SemaphoreHandle_t mbusLogMutex = NULL;

void mbusLogBegin(){
  mbusLogMutex = xSemaphoreCreateMutex();
}

// length of a received frame, a bad frame can be incomplete
uint16_t mbusFrameLength(const uint8_t *data){
  if(data[0] == 0xE5){
    return 1;
  }
  if(data[0] == 0x68){
    return min((uint16_t)(data[1] + 6), (uint16_t)MBUS_DATA_SIZE);
  }
  return 0;
}

// add a poll result to the log, data can be NULL if nothing was received
void mbusLogAdd(uint8_t status, uint8_t address, const uint8_t *data){
  if(mbusLogMutex == NULL || xSemaphoreTake(mbusLogMutex, pdMS_TO_TICKS(100)) != pdTRUE){
    return;
  }
  mbusLogEntry &entry = mbusLog[mbusLogNext];
  entry.time = millis();
  entry.status = status;
  entry.address = address;
  entry.len = (data != NULL) ? mbusFrameLength(data) : 0;
  if(entry.len > 0){
    memcpy(entry.data, data, entry.len);
  }
  mbusLogNext = (mbusLogNext + 1) % MBUS_LOG_SIZE;
  mbusLogCount++;
  xSemaphoreGive(mbusLogMutex);
}

// store the decoded telegram, json is the serialized header + records
void mbusLogSetJson(uint8_t address, const char *json){
  if(strlen(json) >= sizeof(mbusLastJson) - 1){ // probably truncated, no valid JSON
    return;
  }
  if(mbusLogMutex == NULL || xSemaphoreTake(mbusLogMutex, pdMS_TO_TICKS(100)) != pdTRUE){
    return;
  }
  strcpy(mbusLastJson, json);
  mbusLastAddress = address;
  mbusLastTime = millis();
  xSemaphoreGive(mbusLogMutex);
}

// GET /mbus.json, the log newest first and the last decoded telegram
void handleMbusJson(AsyncWebServerRequest *request){
  String out;
  out.reserve(MBUS_LOG_SIZE * (MBUS_DATA_SIZE * 2 + 48) + sizeof(mbusLastJson) + 96);
  if(mbusLogMutex == NULL || xSemaphoreTake(mbusLogMutex, pdMS_TO_TICKS(500)) != pdTRUE){
    request->send(503, "text/plain", "busy");
    return;
  }
  out += "{\"now\":";
  out += millis();
  out += ",\"count\":";
  out += mbusLogCount;
  out += ",\"next\":"; // seconds until the next poll, -1 while a poll is running
  if(mbusLoopStatus == 0){
    unsigned long elapsed = millis() - timerMbus;
    out += (elapsed < userData.mbusInterval) ? (userData.mbusInterval - elapsed) / 1000 : 0;
  }else{
    out += -1;
  }
  out += ",\"log\":[";
  uint8_t entries = min(mbusLogCount, (uint32_t)MBUS_LOG_SIZE);
  for(uint8_t i = 0; i < entries; i++){
    const mbusLogEntry &entry = mbusLog[(mbusLogNext + MBUS_LOG_SIZE - 1 - i) % MBUS_LOG_SIZE];
    if(i > 0){
      out += ',';
    }
    out += "{\"t\":";
    out += entry.time;
    out += ",\"s\":";
    out += entry.status;
    out += ",\"a\":";
    out += entry.address;
    out += ",\"hex\":\"";
    char hex[3];
    for(uint16_t b = 0; b < entry.len; b++){
      snprintf(hex, sizeof(hex), "%02X", entry.data[b]);
      out += hex;
    }
    out += "\"}";
  }
  out += "],\"last\":";
  if(mbusLastJson[0] != 0){
    out += "{\"t\":";
    out += mbusLastTime;
    out += ",\"a\":";
    out += mbusLastAddress;
    out += ",\"data\":";
    out += mbusLastJson;
    out += '}';
  }else{
    out += "null";
  }
  out += '}';
  xSemaphoreGive(mbusLogMutex);
  request->send(200, "application/json", out);
}

// POST /mbus/poll, start the next M-Bus poll right away
void handleMbusPoll(AsyncWebServerRequest *request){
  if(mbusLoopStatus == 0){
    timerMbus = millis() - userData.mbusInterval - 1; // the interval is over, the loop polls at its next run
  }
  request->send(200, "text/plain", "ok");
}
