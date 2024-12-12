float temperature;

OneWire oneWire(5);

//Pass onewire reference to DallasTemperaturez
DallasTemperature sensors(&oneWire);

void beginSensors(){
    sensors.begin();
}

void requestTemperatures(){
   sensors.setWaitForConversion(false);
   sensors.requestTemperatures(); 
}
void readTemperatures(){
    temperature = sensors.getTempCByIndex(0);
}