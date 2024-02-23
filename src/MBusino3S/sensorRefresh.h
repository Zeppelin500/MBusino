
void sensorRefresh1() {           //stößt aktuallisierte Sensorwerte aus den OneWire Sensoren an
  sensor1.requestTemperatures();  // Send the command to get temperatures
  sensor2.requestTemperatures();
  sensor3.requestTemperatures();
  sensor4.requestTemperatures();
  sensor5.requestTemperatures();
  if(userData.extension == 7){
    sensor6.requestTemperatures();
    sensor7.requestTemperatures();
  }
}
void sensorRefresh2() {  //holt die aktuallisierten Sensorwerte aus den OneWire Sensoren
  OW[0] = sensor1.getTempCByIndex(0);
  OW[1] = sensor2.getTempCByIndex(0);
  OW[2] = sensor3.getTempCByIndex(0);
  OW[3] = sensor4.getTempCByIndex(0);
  OW[4] = sensor5.getTempCByIndex(0);
  if(userData.extension == 7){
    OW[5] = sensor6.getTempCByIndex(0);
    OW[6] = sensor7.getTempCByIndex(0);
  }

  OWwO[0] = OW[0] + offset[0];  // Messwert plus Offset from calibration
  OWwO[1] = OW[1] + offset[1];
  OWwO[2] = OW[2] + offset[2];
  OWwO[3] = OW[3] + offset[3];
  OWwO[4] = OW[4] + offset[4];
  if(userData.extension == 7){
    OWwO[5] = OW[5] + offset[5];
    OWwO[6] = OW[6] + offset[6];
  }
  else{
    temperatur = bme.readTemperature();
    druck = bme.readPressure() / 100.0F;
    hoehe = bme.readAltitude(SEALEVELPRESSURE_HPA);
    feuchte = bme.readHumidity();
  }
}
