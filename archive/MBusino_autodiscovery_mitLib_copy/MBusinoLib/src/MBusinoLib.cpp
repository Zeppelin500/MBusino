/*

MBusinoLib, an Arduino M-Bus decoder

Based at the AllWize/mbus-payload library but with much more decode capabilies.

Credits to AllWize!

The MBusinoLib library is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

The MBusinoLib library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with the MBusinoLib library.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "MBusinoLib.h"

// ----------------------------------------------------------------------------

MBusinoLib::MBusinoLib(uint8_t size) : _maxsize(size) {
  _buffer = (uint8_t *) malloc(size);
  _cursor = 0;
}

MBusinoLib::~MBusinoLib(void) {
  free(_buffer);
}

void MBusinoLib::reset(void) {
  _cursor = 0;
}

uint8_t MBusinoLib::getSize(void) {
  return _cursor;
}

uint8_t *MBusinoLib::getBuffer(void) {
  return _buffer;
}

uint8_t MBusinoLib::copy(uint8_t *dst) {
  memcpy(dst, _buffer, _cursor);
  return _cursor;
}

uint8_t MBusinoLib::getError() {
  uint8_t error = _error;
  _error = MBUS_ERROR::NO_ERROR;
  return error;
}

// ----------------------------------------------------------------------------

uint8_t MBusinoLib::decode(uint8_t *buffer, uint8_t size, JsonArray& root) {

	uint8_t count = 0;
	uint8_t index = 0;
	
	while (index < size) {

    	count++;

    	// Decode DIF
    	uint8_t dif = buffer[index++];
    	uint8_t difLeast4bit = (dif & 0x0F);
		  uint8_t difFunctionField = ((dif & 0x30) >> 4);
    	uint8_t len = 0;
    	uint8_t dataCodingType = 0;
    	/*
    	0 -->   no Data
    	1 -->   integer
    	2 -->   bcd
    	3 -->   real
    	4 -->   variable lengs
    	5 -->   special functions
    	6 -->   TimePoint Date&Time Typ F
    	7 -->   TimePoint Date Typ G

    	Length in Bit	Code	    Meaning	        Code	Meaning
    	0	            0000	    No data	        1000	Selection for Readout
    	8	            0001	    8 Bit Integer	1001	2 digit BCD
    	16	            0010	    16 Bit Integer	1010	4 digit BCD
    	24	            0011	    24 Bit Integer	1011	6 digit BCD
    	32	            0100	    32 Bit Integer	1100	8 digit BCD
    	32 / N	        0101	    32 Bit Real	    1101	variable length
    	48	            0110	    48 Bit Integer	1110	12 digit BCD
    	64	            0111	    64 Bit Integer	1111	Special Functions 
    	*/
    
    	switch(difLeast4bit){
        	case 0x00:  //No data
            	len = 0;
            	dataCodingType = 0;
            	break;        
        	case 0x01:  //0001	    8 Bit Integer
            	len = 1;
            	dataCodingType = 1;
            	break;
        	case 0x02:  //0010	    16 Bit Integer
            	len = 2;
            	dataCodingType = 1;
            	break;
        	case 0x03:  //0011	    24 Bit Integer
            	len = 3;
            	dataCodingType = 1;
            	break;
        	case 0x04:  //0100	    32 Bit Integer
            	len = 4;
            	dataCodingType = 1;
            	break;
        	case 0x05:  //0101	    32 Bit Real
            	len = 4;
            	dataCodingType = 3;
            	break;        
        	case 0x06:  //0110	    48 Bit Integer
            	len = 6;
            	dataCodingType = 1;
            	break; 
        	case 0x07:  //0111	    64 Bit Integer
            	len = 8;
            	dataCodingType = 1;
            	break;
        	case 0x08:  //not supported
            	len = 0;
            	dataCodingType = 0;
            	break;
        	case 0x09:  //1001 2 digit BCD
            	len = 1;
            	dataCodingType = 2;
            	break;
        	case 0x0A:  //1010 4 digit BCD
            	len = 2;
            	dataCodingType = 2;
            	break;
        	case 0x0B:  //1011 6 digit BCD
            	len = 3;
            	dataCodingType = 2;
            	break;
        	case 0x0C:  //1100 8 digit BCD
            	len = 4;
            	dataCodingType = 2;
            	break;
        	case 0x0D:  //1101	variable length
            	len = 0;
            	dataCodingType = 4;
            	break;
        	case 0x0E:  //1110	12 digit BCD
            	len = 6;
            	dataCodingType = 2;
            	break;
        	case 0x0F:  //1111	Special Functions
            	len = 0;
            	dataCodingType = 5;
            	break;
    	}
		char stringFunctionField[5];
		switch(difFunctionField){
			case 0:
				strcpy(stringFunctionField, ""); //current
				break;
			case 1:
				strcpy(stringFunctionField, "_max");
				break;
			case 2:
				strcpy(stringFunctionField, "_min");
				break;
			case 3:
				strcpy(stringFunctionField, "_err");
				break;
		}
			
			
    // handle DIFE to prevent stumble if a DIFE is used
    uint16_t storageNumber = 0;
    if((dif & 0x40) == 0x40){
      storageNumber = 1;
    }
    uint8_t difeNumber = 0;
    uint8_t dife[10] = {0};
    bool ifDife = ((dif & 0x80) == 0x80); //check if the first bit of DIF marked as "DIFE is following" 
    while(ifDife) {        
        difeNumber ++;
        dife[difeNumber] = buffer[index];
      	ifDife = false;
      	ifDife = ((buffer[index] & 0x80) == 0x80); //check if after the DIFE another DIFE is following 
        index++;
    } 
    uint8_t subUnit = 0;
    uint8_t tariff = 0;
    
    for(uint8_t i = 0; difeNumber > 0 && i<=difeNumber; i++){    
      if(i==0){
        storageNumber = storageNumber | ((dife[i+1] & 0x0F) << 1);
      }
      else{
        storageNumber = storageNumber | ((dife[i+1] & 0x0F) << (4*i));
      }
      subUnit = subUnit | (((dife[i+1] & 0x40) >> 6) << i);
      tariff = tariff | (((dife[i+1] & 0x30) >> 4) << (2*i));
    }
    
    //  End of DIFE handling
 

    // Get VIF(E)
    uint32_t vif = 0;
    uint8_t vifarray[4] = {0};
    uint8_t vifcounter = 0;
    do {
      if (index == size) {
        _error = MBUS_ERROR::BUFFER_OVERFLOW;
        return 0;
      }
      vifarray[vifcounter] = buffer[index];
      vifcounter++;
      vif = (vif << 8) + buffer[index++];
    } while ((vif & 0x80) == 0x80);

    if(((vifarray[0] & 0x80) == 0x80) && (vifarray[1] != 0x00 ) && (vifarray[0] != 0XFD) && (vifarray[0] != 0XFC)&& (vifarray[0] != 0XFB)){
      vif = (vifarray[0] & 0x7F);
    }

    // Find definition
    int8_t def = _findDefinition(vif);
    if (def < 0) {
      _error = MBUS_ERROR::UNSUPPORTED_VIF;
      return 0;
    }
    
    char customVIF[10] = {0}; 
		bool ifcustomVIF = false;	
		
    if((vif & 0x6D) == 0x6D){  // TimePoint Date&Time TypF
      dataCodingType = 6;   
    }
    else if((vif & 0x6C) == 0x6C){  // TimePoint Date TypG
      dataCodingType = 7;   
    }
    else if((vif & 0xFF00) == 0xFC00){   // VIF 0xFC --> Customized VIF as ASCII
      uint8_t customVIFlen = (vif & 0x00FF);
      char vifBuffer[customVIFlen];// = {0};    
      for (uint8_t i = 0; i<=customVIFlen; i++) {
        vifBuffer[i] = buffer[index]; 
				index++;
      }  
      strncpy(customVIF, vifBuffer,customVIFlen );  
			ifcustomVIF = true;	
    } 	  

    // Check buffer overflow
    if (index + len > size) {
      _error = MBUS_ERROR::BUFFER_OVERFLOW;
      return 0;
    }

    // read value
    int16_t value16 = 0;  	// int16_t to notice negative values at 2 byte data
    int32_t value32 = 0;	// int32_t to notice negative values at 4 byte data	  
    int64_t value = 0;

    float valueFloat = 0; //real value
	  
	  
	  uint8_t date[len]; // ={0};
	  char datestring[16] = {0}; //needed for simple formatted dates 
	  char datestring2[30] = {0};//needed for extensive formatted dates
    char valueString [30] = {0}; // contain the ASCII value at variable length ascii values and formatted dates
    bool switchAgain = false; // repeat the switch(dataCodingType) at variable length coding
    bool negative = false;  // set a negative flag for negate the value
    uint8_t asciiValue = 0; // 0 = double, 1 = ASCII, 2 = both;

    do{
      switchAgain = false;
      switch(dataCodingType){
        case 0:    //no Data
              
          break;
        case 1:    //integer
          if(len==2){
            for (uint8_t i = 0; i<len; i++) {
              value16 = (value16 << 8) + buffer[index + len - i - 1];
            }
            value = (int64_t)value16;
          }
          else if(len==4){
            for (uint8_t i = 0; i<len; i++) {
              value32 = (value32 << 8) + buffer[index + len - i - 1];
            }	
            value = (int64_t)value32;
          }			
          else{
            for (uint8_t i = 0; i<len; i++) {
              value = (value << 8) + buffer[index + len - i - 1];
            }            
          }
          break;
        case 2:    //bcd
          if(len==2){
            for (uint8_t i = 0; i<len; i++) {
              uint8_t byte = buffer[index + len - i - 1];
              value16 = (value16 * 100) + ((byte >> 4) * 10) + (byte & 0x0F);
            }
            value = (int64_t)value16;
          }
          else if(len==4){
            for (uint8_t i = 0; i<len; i++) {
              uint8_t byte = buffer[index + len - i - 1];
              value32 = (value32 * 100) + ((byte >> 4) * 10) + (byte & 0x0F);
            }
            value = (int64_t)value32;				 
          }
          else{
            for (uint8_t i = 0; i<len; i++) {
              uint8_t byte = buffer[index + len - i - 1];
              value = (value * 100) + ((byte >> 4) * 10) + (byte & 0x0F);
            }           
          } 
          if(negative == true){
            value = value * -1;
          }    
          break;
        case 3:    //real
          for (uint8_t i = 0; i<len; i++) {
            value = (value << 8) + buffer[index + len - i - 1];
          } 
          memcpy(&valueFloat, &value, sizeof(valueFloat));
          break;
        case 4:    //variable lengs
          /*
          Variable Length:
          With data field = `1101b` several data types with variable length can be used. The length of
          the data is given with the first byte of data, which is here called LVAR.
          LVAR = 00h .. BFh :ASCII string with LVAR characters
          LVAR = C0h .. CFh :positive BCD number with (LVAR - C0h) • 2 digits
          LVAR = D0h .. DFH :negative BCD number with (LVAR - D0h) • 2 digits
          LVAR = E0h .. EFh :binary number with (LVAR - E0h) bytes
          LVAR = F0h .. FAh :floating point number with (LVAR - F0h) bytes [to bedefined]
          LVAR = FBh .. FFh :Reserved
          */
          // only ASCII string supported
          if(buffer[index] >= 0x00 || buffer[index] <= 0xBF){ //ASCII string with LVAR characters
            len = buffer[index];
            index ++;
            char charBuffer[len]; // = {0};        
            for (uint8_t i = 0; i<=len; i++) { // evtl das "=" löschen
              charBuffer[i] = buffer[index + (len-i-1)]; 
            }  
            strncpy(valueString, charBuffer,len );  
            asciiValue = 1;	
          }
          else if(buffer[index] >= 0xC0 && buffer[index] <= 0xCF){ //positive BCD number with (LVAR - C0h) • 2 digits
            len = buffer[index] - 0xC0;
            index ++;
            dataCodingType = 2;
            switchAgain = true;
            break;
          }
          else if(buffer[index] >= 0xD0 && buffer[index] <= 0xDF){ //negative BCD number with (LVAR - D0h) • 2 digits
            len = buffer[index] - 0xD0;
            index ++;
            dataCodingType = 2;
            switchAgain = true;
            negative = true;
            break;
          }    
          else if(buffer[index] >= 0xE0 && buffer[index] <= 0xEF){ //binary number with (LVAR - E0h) bytes
            len = buffer[index] - 0xE0;
            index ++;            
            dataCodingType = 1;
            switchAgain = true;
            break;
          }    
          else if(buffer[index] >= 0xF0 && buffer[index] <= 0xFA){ //floating point number with (LVAR - F0h) bytes [to bedefined]
            len = buffer[index] - 0xF0;
            index ++;            
            dataCodingType = 3;
            switchAgain = true;
            break;
          } 

          break;
        case 5:    //special functions
          
          break;
        case 6:    //TimePoint Date&Time Typ F
          for (uint8_t i = 0; i<len; i++) {
            date[i] =  buffer[index + i];
          }            			
          if ((date[0] & 0x80) != 0) {    // Time valid ?
            //out_len = snprintf(output, output_size, "invalid");
            break;
          }
          snprintf(datestring, 24, "20%02d%02d%02d%02d%02d",
            ((date[2] & 0xE0) >> 5) | ((date[3] & 0xF0) >> 1), // year
            date[3] & 0x0F, // mon
            date[2] & 0x1F, // mday
            date[1] & 0x1F, // hour
            date[0] & 0x3F // min
          );  
        
          snprintf(datestring2, 24, "20%02d-%02d-%02d %02d:%02d:00",
            ((date[2] & 0xE0) >> 5) | ((date[3] & 0xF0) >> 1), // year
            date[3] & 0x0F, // mon
            date[2] & 0x1F, // mday
            date[1] & 0x1F, // hour
            date[0] & 0x3F // min
          );	
          value = atof( datestring);
          strcpy(valueString, datestring2);
          asciiValue = 2;	
          break;
        case 7:    //TimePoint Date Typ G
          for (uint8_t i = 0; i<len; i++) {
            date[i] =  buffer[index + i];
          }            
        
          if ((date[1] & 0x0F) > 12) {    // Time valid ?
            //out_len = snprintf(output, output_size, "invalid");
            break;
          }
          snprintf(datestring, 12, "20%02d%02d%02d",
            ((date[0] & 0xE0) >> 5) | ((date[1] & 0xF0) >> 1), // year
            date[1] & 0x0F, // mon
            date[0] & 0x1F  // mday
          );
          snprintf(datestring2, 12, "20%02d-%02d-%02d",
            ((date[0] & 0xE0) >> 5) | ((date[1] & 0xF0) >> 1), // year
            date[1] & 0x0F, // mon
            date[0] & 0x1F  // mday
          );			
          value = atof( datestring);
          strcpy(valueString, datestring2);
          asciiValue = 2;	
          break;
        default:
          break;
      }
    }while (switchAgain == true);

    index += len;

    // scaled value
    double scaled = 0;
	  int8_t scalar = vif_defs[def].scalar + vif - vif_defs[def].base;	  
    if(dataCodingType == 3){
      scaled = valueFloat;
    }
    else{
      scaled = value;
      for (int8_t i=0; i<scalar; i++) scaled *= 10;
      for (int8_t i=scalar; i<0; i++) scaled /= 10;
    }

    if(vifarray[1] == 0x7D){ // from table "Codes for Value Information Field Extension" E111 1101	Multiplicative correction factor: 1000
      scaled = scaled * 1000;
    }
    
    // Init object
    JsonObject data = root.add<JsonObject>();
    //data["vif"] = String("0x" + String(vif,HEX));
    data["code"] = vif_defs[def].code;

    if(asciiValue != 1){ //0 = double, 1 = ASCII, 2 = both;
      //data["scalar"] = scalar;
      //data["value_raw"] = value;
      data["value_scaled"] = scaled; 
    }
    if(asciiValue > 0){ //0 = double, 1 = ASCII, 2 = both;
      data["value_string"] = String(valueString); 
    }
    if(ifcustomVIF == true){
		  data["units"] = String(customVIF);
	  }
    else if(getCodeUnits(vif_defs[def].code)!=0){
      data["units"] = String(getCodeUnits(vif_defs[def].code));
    }
    data["name"] = String(getCodeName(vif_defs[def].code)+String(stringFunctionField));
    if(subUnit > 0){
      data["subUnit"] = subUnit;
    }
    if(storageNumber > 0){
      data["storage"] = storageNumber;
    }
    if(tariff > 0){
      data["tariff"] = tariff;
    }    
    /* // only for debug
    data["difes"] = difeNumber;
    if(difeNumber > 0){
      data["dife1"] = dife[1];
      data["dife2"] = dife[2];
    }
    */
	  if(buffer[index] == 0x0F ||buffer[index] == 0x1F){ // If last byte 1F/0F --> More records follow in next telegram
		  index++;
	  }	
    yield();        
  }
	return count;
}

const char * MBusinoLib::getCodeUnits(uint8_t code) {
  switch (code) {

    case MBUS_CODE::ENERGY_WH:
      return "Wh";
    
    case MBUS_CODE::ENERGY_J:
      return "J";

    case MBUS_CODE::VOLUME_M3: 
      return "m³";

    case MBUS_CODE::MASS_KG: 
      return "kg";

    case MBUS_CODE::ON_TIME_S: 
    case MBUS_CODE::OPERATING_TIME_S: 
    case MBUS_CODE::AVG_DURATION_S:
    case MBUS_CODE::ACTUAL_DURATION_S:
      return "s";

    case MBUS_CODE::ON_TIME_MIN: 
    case MBUS_CODE::OPERATING_TIME_MIN: 
    case MBUS_CODE::AVG_DURATION_MIN:
    case MBUS_CODE::ACTUAL_DURATION_MIN:
      return "min";
      
    case MBUS_CODE::ON_TIME_H: 
    case MBUS_CODE::OPERATING_TIME_H: 
    case MBUS_CODE::AVG_DURATION_H:
    case MBUS_CODE::ACTUAL_DURATION_H:
      return "h";
      
    case MBUS_CODE::ON_TIME_DAYS: 
    case MBUS_CODE::OPERATING_TIME_DAYS: 
    case MBUS_CODE::AVG_DURATION_DAYS:
    case MBUS_CODE::ACTUAL_DURATION_DAYS:
      return "d";
      
    case MBUS_CODE::POWER_W:
    case MBUS_CODE::MAX_POWER_W: 
      return "W";
      
    case MBUS_CODE::POWER_J_H: 
      return "J/h";
      
    case MBUS_CODE::VOLUME_FLOW_M3_H: 
      return "m³/h";
      
    case MBUS_CODE::VOLUME_FLOW_M3_MIN:
      return "m³/min";
      
    case MBUS_CODE::VOLUME_FLOW_M3_S: 
      return "m³/s";
      
    case MBUS_CODE::MASS_FLOW_KG_H: 
      return "kg/h";
      
    case MBUS_CODE::FLOW_TEMPERATURE_C: 
    case MBUS_CODE::RETURN_TEMPERATURE_C: 
    case MBUS_CODE::EXTERNAL_TEMPERATURE_C: 
    case MBUS_CODE::TEMPERATURE_LIMIT_C:
      return "°C";

    case MBUS_CODE::TEMPERATURE_DIFF_K: 
      return "K";

    case MBUS_CODE::PRESSURE_BAR: 
      return "bar";

    case MBUS_CODE::TIME_POINT_DATE:
      return "YYYYMMDD";  

    case MBUS_CODE::TIME_POINT_DATETIME:
      return "YYYYMMDDhhmm";  

    case MBUS_CODE::BAUDRATE_BPS:
      return "bit/s";

    case MBUS_CODE::VOLTS: 
      return "V";

    case MBUS_CODE::AMPERES: 
      return "A";
      
    case MBUS_CODE::VOLUME_FT3:
      return "ft³";

    case MBUS_CODE::VOLUME_GAL: 
      return "gal";
      
    case MBUS_CODE::VOLUME_FLOW_GAL_M: 
      return "gal/min";
      
    case MBUS_CODE::VOLUME_FLOW_GAL_H: 
      return "gal/h";
      
    case MBUS_CODE::FLOW_TEMPERATURE_F:
    case MBUS_CODE::RETURN_TEMPERATURE_F:
    case MBUS_CODE::TEMPERATURE_DIFF_F:
    case MBUS_CODE::EXTERNAL_TEMPERATURE_F:
    case MBUS_CODE::TEMPERATURE_LIMIT_F:
      return "°F";

    case MBUS_CODE::CUSTOMIZED_VIF:
      return "X";      

    default:
      break; 

  }

  return 0;

}

const char * MBusinoLib::getCodeName(uint8_t code) {
  switch (code) {

    case MBUS_CODE::ENERGY_WH:
    case MBUS_CODE::ENERGY_J:
      return "energy";
    
    case MBUS_CODE::VOLUME_M3: 
    case MBUS_CODE::VOLUME_FT3:
    case MBUS_CODE::VOLUME_GAL: 
      return "volume";

    case MBUS_CODE::MASS_KG: 
      return "mass";

    case MBUS_CODE::ON_TIME_S: 
    case MBUS_CODE::ON_TIME_MIN: 
    case MBUS_CODE::ON_TIME_H: 
    case MBUS_CODE::ON_TIME_DAYS: 
      return "on_time";
    
    case MBUS_CODE::OPERATING_TIME_S: 
    case MBUS_CODE::OPERATING_TIME_MIN: 
    case MBUS_CODE::OPERATING_TIME_H: 
    case MBUS_CODE::OPERATING_TIME_DAYS: 
      return "operating_time";
    
    case MBUS_CODE::AVG_DURATION_S:
    case MBUS_CODE::AVG_DURATION_MIN:
    case MBUS_CODE::AVG_DURATION_H:
    case MBUS_CODE::AVG_DURATION_DAYS:
      return "avg_duration";
    
    case MBUS_CODE::ACTUAL_DURATION_S:
    case MBUS_CODE::ACTUAL_DURATION_MIN:
    case MBUS_CODE::ACTUAL_DURATION_H:
    case MBUS_CODE::ACTUAL_DURATION_DAYS:
      return "actual_duration";

    case MBUS_CODE::POWER_W:
    case MBUS_CODE::MAX_POWER_W: 
    case MBUS_CODE::POWER_J_H: 
      return "power";
      
    case MBUS_CODE::VOLUME_FLOW_M3_H: 
    case MBUS_CODE::VOLUME_FLOW_M3_MIN:
    case MBUS_CODE::VOLUME_FLOW_M3_S: 
    case MBUS_CODE::VOLUME_FLOW_GAL_M: 
    case MBUS_CODE::VOLUME_FLOW_GAL_H: 
      return "volume_flow";

    case MBUS_CODE::MASS_FLOW_KG_H: 
      return "mass_flow";

    case MBUS_CODE::FLOW_TEMPERATURE_C: 
    case MBUS_CODE::FLOW_TEMPERATURE_F:
      return "flow_temperature";

    case MBUS_CODE::RETURN_TEMPERATURE_C: 
    case MBUS_CODE::RETURN_TEMPERATURE_F:
      return "return_temperature";

    case MBUS_CODE::EXTERNAL_TEMPERATURE_C: 
    case MBUS_CODE::EXTERNAL_TEMPERATURE_F:
      return "external_temperature";

    case MBUS_CODE::TEMPERATURE_LIMIT_C:
    case MBUS_CODE::TEMPERATURE_LIMIT_F:
      return "temperature_limit";

    case MBUS_CODE::TEMPERATURE_DIFF_K: 
    case MBUS_CODE::TEMPERATURE_DIFF_F:
      return "temperature_diff";

    case MBUS_CODE::PRESSURE_BAR: 
      return "pressure";

    case MBUS_CODE::TIME_POINT_DATE:
    case MBUS_CODE::TIME_POINT_DATETIME:
      return "time_point";

    case MBUS_CODE::BAUDRATE_BPS:
      return "baudrate";

    case MBUS_CODE::VOLTS: 
      return "voltage";

    case MBUS_CODE::AMPERES: 
      return "current";
      
    case MBUS_CODE::FABRICATION_NUMBER: 
      return "fab_number";

    case MBUS_CODE::BUS_ADDRESS: 
      return "bus_address";

    case MBUS_CODE::CREDIT: 
      return "credit";

    case MBUS_CODE::DEBIT: 
      return "debit";

    case MBUS_CODE::ACCESS_NUMBER: 
      return "access_number";

    case MBUS_CODE::MANUFACTURER: 
      return "manufacturer";

    case MBUS_CODE::MODEL_VERSION: 
      return "model_version";

    case MBUS_CODE::HARDWARE_VERSION: 
      return "hardware_version";

    case MBUS_CODE::FIRMWARE_VERSION: 
      return "firmware_version";

    case MBUS_CODE::SOFTWARE_VERSION: 
      return "software_version"; //neu

    case MBUS_CODE::CUSTOMER_LOCATION: 
      return "customer_location"; //neu      

    case MBUS_CODE::CUSTOMER: 
      return "customer";
  
    case MBUS_CODE::ERROR_FLAGS: 
      return "error_flags";
  
    case MBUS_CODE::ERROR_MASK: 
      return "error_mask";
  
    case MBUS_CODE::DIGITAL_OUTPUT: 
      return "digital_output";
  
    case MBUS_CODE::DIGITAL_INPUT: 
      return "digital_input";
  
    case MBUS_CODE::RESPONSE_DELAY_TIME: 
      return "response_delay";
  
    case MBUS_CODE::RETRY: 
      return "retry";
  
    case MBUS_CODE::GENERIC: 
      return "generic";
  
    case MBUS_CODE::RESET_COUNTER: 
    case MBUS_CODE::CUMULATION_COUNTER: 
      return "counter";

    case MBUS_CODE::CUSTOMIZED_VIF: 
      return "customized_vif";  
    default:
        break; 

  }

  return "";

}

const char * MBusinoLib::getDeviceClass(uint8_t code) {
  switch (code) {

    case MBUS_CODE::ENERGY_WH:
    case MBUS_CODE::ENERGY_J:
      return "energy";
    
    case MBUS_CODE::VOLUME_M3: 
    case MBUS_CODE::VOLUME_FT3:
    case MBUS_CODE::VOLUME_GAL: 
      return "volume";

    case MBUS_CODE::MASS_KG: 
      return "weight";

    case MBUS_CODE::OPERATING_TIME_S: 
    case MBUS_CODE::OPERATING_TIME_MIN: 
    case MBUS_CODE::OPERATING_TIME_H: 
    case MBUS_CODE::OPERATING_TIME_DAYS: 
    case MBUS_CODE::ON_TIME_S: 
    case MBUS_CODE::ON_TIME_MIN: 
    case MBUS_CODE::ON_TIME_H: 
    case MBUS_CODE::ON_TIME_DAYS: 
    case MBUS_CODE::AVG_DURATION_S:
    case MBUS_CODE::AVG_DURATION_MIN:
    case MBUS_CODE::AVG_DURATION_H:
    case MBUS_CODE::AVG_DURATION_DAYS:
    case MBUS_CODE::ACTUAL_DURATION_S:
    case MBUS_CODE::ACTUAL_DURATION_MIN:
    case MBUS_CODE::ACTUAL_DURATION_H:
    case MBUS_CODE::ACTUAL_DURATION_DAYS:
      return "duration";

    case MBUS_CODE::POWER_W:
    case MBUS_CODE::MAX_POWER_W: 
    case MBUS_CODE::POWER_J_H: 
      return "power";
      
    case MBUS_CODE::VOLUME_FLOW_M3_H: 
    case MBUS_CODE::VOLUME_FLOW_M3_MIN:
    case MBUS_CODE::VOLUME_FLOW_M3_S: 
    case MBUS_CODE::VOLUME_FLOW_GAL_M: 
    case MBUS_CODE::VOLUME_FLOW_GAL_H: 
      return "volume_flow_rate";

    case MBUS_CODE::MASS_FLOW_KG_H: // no Unit or device Class in HA defind
      return "";

    case MBUS_CODE::FLOW_TEMPERATURE_C: 
    case MBUS_CODE::FLOW_TEMPERATURE_F:
    case MBUS_CODE::RETURN_TEMPERATURE_C: 
    case MBUS_CODE::RETURN_TEMPERATURE_F:
    case MBUS_CODE::EXTERNAL_TEMPERATURE_C: 
    case MBUS_CODE::EXTERNAL_TEMPERATURE_F:
    case MBUS_CODE::TEMPERATURE_LIMIT_C:
    case MBUS_CODE::TEMPERATURE_LIMIT_F:
    case MBUS_CODE::TEMPERATURE_DIFF_K: 
    case MBUS_CODE::TEMPERATURE_DIFF_F:
      return "temperature";

    case MBUS_CODE::PRESSURE_BAR: 
      return "pressure";

    case MBUS_CODE::TIME_POINT_DATE:
    case MBUS_CODE::TIME_POINT_DATETIME:
      return "";

    case MBUS_CODE::BAUDRATE_BPS:
      return "data_rate";

    case MBUS_CODE::VOLTS: 
      return "voltage";

    case MBUS_CODE::AMPERES: 
      return "current";
      
    case MBUS_CODE::FABRICATION_NUMBER: 
    case MBUS_CODE::BUS_ADDRESS: 
    case MBUS_CODE::CREDIT: 
    case MBUS_CODE::DEBIT: 
    case MBUS_CODE::ACCESS_NUMBER: 
    case MBUS_CODE::MANUFACTURER: 
    case MBUS_CODE::MODEL_VERSION: 
    case MBUS_CODE::HARDWARE_VERSION: 
    case MBUS_CODE::FIRMWARE_VERSION: 
    case MBUS_CODE::SOFTWARE_VERSION: 
    case MBUS_CODE::CUSTOMER_LOCATION: 
    case MBUS_CODE::CUSTOMER: 
    case MBUS_CODE::ERROR_FLAGS: 
    case MBUS_CODE::ERROR_MASK: 
    case MBUS_CODE::DIGITAL_OUTPUT: 
    case MBUS_CODE::DIGITAL_INPUT: 
    case MBUS_CODE::RESPONSE_DELAY_TIME: 
    case MBUS_CODE::RETRY:   
    case MBUS_CODE::GENERIC: 
    case MBUS_CODE::RESET_COUNTER: 
    case MBUS_CODE::CUMULATION_COUNTER: 
    case MBUS_CODE::CUSTOMIZED_VIF: 
      return "";  
    default:
        break;

  }
  return "";
}

const char * MBusinoLib::getStateClass(uint8_t code) {
  switch (code) {

    case MBUS_CODE::ENERGY_WH:
    case MBUS_CODE::ENERGY_J:
    case MBUS_CODE::AMPERES: 
    case MBUS_CODE::MASS_KG: 
    case MBUS_CODE::POWER_W:
    case MBUS_CODE::MAX_POWER_W: 
    case MBUS_CODE::POWER_J_H: 
    case MBUS_CODE::VOLUME_FLOW_M3_H: 
    case MBUS_CODE::VOLUME_FLOW_M3_MIN:
    case MBUS_CODE::VOLUME_FLOW_M3_S: 
    case MBUS_CODE::VOLUME_FLOW_GAL_M: 
    case MBUS_CODE::VOLUME_FLOW_GAL_H: 
    case MBUS_CODE::MASS_FLOW_KG_H: 
    case MBUS_CODE::FLOW_TEMPERATURE_C: 
    case MBUS_CODE::FLOW_TEMPERATURE_F:
    case MBUS_CODE::RETURN_TEMPERATURE_C: 
    case MBUS_CODE::RETURN_TEMPERATURE_F:
    case MBUS_CODE::EXTERNAL_TEMPERATURE_C: 
    case MBUS_CODE::EXTERNAL_TEMPERATURE_F:
    case MBUS_CODE::TEMPERATURE_LIMIT_C:
    case MBUS_CODE::TEMPERATURE_LIMIT_F:
    case MBUS_CODE::TEMPERATURE_DIFF_K: 
    case MBUS_CODE::TEMPERATURE_DIFF_F:
    case MBUS_CODE::PRESSURE_BAR: 
    case MBUS_CODE::BAUDRATE_BPS:
    case MBUS_CODE::VOLTS: 
      return "measurement";
    
    case MBUS_CODE::VOLUME_M3: 
    case MBUS_CODE::VOLUME_FT3:
    case MBUS_CODE::VOLUME_GAL: 
    case MBUS_CODE::OPERATING_TIME_S: 
    case MBUS_CODE::OPERATING_TIME_MIN: 
    case MBUS_CODE::OPERATING_TIME_H: 
    case MBUS_CODE::OPERATING_TIME_DAYS: 
    case MBUS_CODE::ON_TIME_S: 
    case MBUS_CODE::ON_TIME_MIN: 
    case MBUS_CODE::ON_TIME_H: 
    case MBUS_CODE::ON_TIME_DAYS: 
    case MBUS_CODE::AVG_DURATION_S:
    case MBUS_CODE::AVG_DURATION_MIN:
    case MBUS_CODE::AVG_DURATION_H:
    case MBUS_CODE::AVG_DURATION_DAYS:
    case MBUS_CODE::ACTUAL_DURATION_S:
    case MBUS_CODE::ACTUAL_DURATION_MIN:
    case MBUS_CODE::ACTUAL_DURATION_H:
    case MBUS_CODE::ACTUAL_DURATION_DAYS:
    case MBUS_CODE::TIME_POINT_DATE:
    case MBUS_CODE::TIME_POINT_DATETIME:
    case MBUS_CODE::FABRICATION_NUMBER: 
    case MBUS_CODE::BUS_ADDRESS: 
    case MBUS_CODE::CREDIT: 
    case MBUS_CODE::DEBIT: 
    case MBUS_CODE::ACCESS_NUMBER: 
    case MBUS_CODE::MANUFACTURER: 
    case MBUS_CODE::MODEL_VERSION: 
    case MBUS_CODE::HARDWARE_VERSION: 
    case MBUS_CODE::FIRMWARE_VERSION: 
    case MBUS_CODE::SOFTWARE_VERSION: 
    case MBUS_CODE::CUSTOMER_LOCATION: 
    case MBUS_CODE::CUSTOMER: 
    case MBUS_CODE::ERROR_FLAGS: 
    case MBUS_CODE::ERROR_MASK: 
    case MBUS_CODE::DIGITAL_OUTPUT: 
    case MBUS_CODE::DIGITAL_INPUT: 
    case MBUS_CODE::RESPONSE_DELAY_TIME: 
    case MBUS_CODE::RETRY:   
    case MBUS_CODE::GENERIC: 
    case MBUS_CODE::RESET_COUNTER: 
    case MBUS_CODE::CUMULATION_COUNTER: 
    case MBUS_CODE::CUSTOMIZED_VIF: 
      return "total";
    default:
        break;
  }
  return "";
}

// ----------------------------------------------------------------------------

int8_t MBusinoLib::_findDefinition(uint32_t vif) {
  
  for (uint8_t i=0; i<MBUS_VIF_DEF_NUM; i++) {
    vif_def_type vif_def = vif_defs[i];
    if ((vif_def.base <= vif) && (vif < (vif_def.base + vif_def.size))) {
      return i;
    }
  }
  
  return -1;

}

uint32_t MBusinoLib::_getVIF(uint8_t code, int8_t scalar) {

  for (uint8_t i=0; i<MBUS_VIF_DEF_NUM; i++) {
    vif_def_type vif_def = vif_defs[i];
    if (code == vif_def.code) {
      if ((vif_def.scalar <= scalar) && (scalar < (vif_def.scalar + vif_def.size))) {
        return vif_def.base + (scalar - vif_def.scalar);
      }
    }
  }
  
  return 0xFF; // this is not a valid VIF

}
