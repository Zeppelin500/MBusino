#define MBUS_BAUD_RATE 2400
#define MBUS_ADDRESS 0xFE  // brodcast
#define MBUS_TIMEOUT 1000  // milliseconds
#define MBUS_DATA_SIZE 255
#define MBUS_GOOD_FRAME true
#define MBUS_BAD_FRAME false

#define MBUS_FRAME_SHORT_START          0x10
#define MBUS_FRAME_LONG_START           0x68
#define MBUS_FRAME_STOP                 0x16

#define MBUS_CONTROL_MASK_SND_NKE       0x40
#define MBUS_CONTROL_MASK_DIR_M2S       0x40
#define MBUS_ADDRESS_NETWORK_LAYER      0xFE



void mbus_short_frame(byte address, byte C_field) {
  byte data[6];

  data[0] = 0x10;
  data[1] = C_field;
  data[2] = address;
  data[3] = data[1] + data[2];
  data[4] = 0x16;
  data[5] = '\0';
  #if defined(ESP8266)
  Serial.write((char *)data,5);
  #elif defined(ESP32)
  MbusSerial.write((char *)data,5);
  #endif
}

void mbus_request_data(byte address) {
  mbus_short_frame(address, 0x5b);
}

bool mbus_get_response(byte *pdata, unsigned char len_pdata) {
  byte bid = 0;             // current byte of response frame
  byte bid_end = 255;       // last byte of frame calculated from length byte sent
  byte bid_checksum = 255;  // checksum byte of frame (next to last)
  byte len = 0;
  byte checksum = 0;
  bool long_frame_found = false;
  bool complete_frame = false;
  bool frame_error = false;
  uint16_t j = 0;

  while (!frame_error && !complete_frame){
    j++;
    if(j>255){
      frame_error = true;
    }
  #if defined(ESP8266)
  while (Serial.available()) {
      byte received_byte = (byte)Serial.read();
  #elif defined(ESP32)
  while (MbusSerial.available()) {
      byte received_byte = (byte)MbusSerial.read();
  #endif
      // Try to skip noise
      if (bid == 0 && received_byte != 0xE5 && received_byte != 0x68) {
        continue;
      }

      if (bid > len_pdata) {
        return MBUS_BAD_FRAME;
      }
      pdata[bid] = received_byte;

      // Single Character (ACK)
      if (bid == 0 && received_byte == 0xE5) {
        return MBUS_GOOD_FRAME;
      }

      // Long frame start
      if (bid == 0 && received_byte == 0x68) {
        long_frame_found = true;
      }

      if (long_frame_found) {
        // 2nd byte is the frame length
        if (bid == 1) {
          len = received_byte;
          bid_end = len + 4 + 2 - 1;
          bid_checksum = bid_end - 1;
        }

        if (bid == 2 && received_byte != len) {  // 3rd byte is also length, check that its the same as 2nd byte
          frame_error = true;
        }
        if (bid == 3 && received_byte != 0x68) {
          ;  // 4th byte is the start byte again
          frame_error = true;
        }
        if (bid > 3 && bid < bid_checksum) checksum += received_byte;  // Increment checksum during data portion of frame

        if (bid == bid_checksum && received_byte != checksum) {  // Validate checksum
          frame_error = true;
        }
        if (bid == bid_end && received_byte == 0x16) {  // Parse frame if still valid
          complete_frame = true;
        }
      }
      bid++;
      yield();
    }
  }

  if (complete_frame && !frame_error) {
    return MBUS_GOOD_FRAME;
  } else {
    return MBUS_BAD_FRAME;
  }
}

void mbus_normalize(byte address) {
  mbus_short_frame(address,0x40);
}

void mbus_clearRXbuffer(){
  #if defined(ESP8266)
  while (Serial.available()) {
      byte received_byte = (byte)Serial.read();
  #elif defined(ESP32)
  while (MbusSerial.available()) {
      byte received_byte = (byte)MbusSerial.read();
  #endif
  }
}

void mbus_set_address(byte oldaddress, byte newaddress) {
 
  byte data[13];
 
  data[0] = MBUS_FRAME_LONG_START;
  data[1] = 0x06;
  data[2] = 0x06;
  data[3] = MBUS_FRAME_LONG_START;
  
  data[4] = 0x53;
  data[5] = oldaddress;
  data[6] = 0x51;
  
  data[7] = 0x01;         // DIF [EXT0, LSB0, FN:00, DATA 1 8BIT INT]
  data[8] = 0x7A;         // VIF 0111 1010 bus address
  data[9] = newaddress;   // DATA new address
  
  data[10] = data[4]+data[5]+data[6]+data[7]+data[8]+data[9];
  data[11] = 0x16;
  data[12] = '\0';
  
  #if defined(ESP8266)
  Serial.write((char*)data,12);
  #elif defined(ESP32)
  MbusSerial.write((char*)data,12);
  #endif
}