#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "temperature.h"
#include "timerconfig.h"
#include "ESP8266TimerInterrupt.h"
#include "crc16.h"
#include "horus_l2.h"
#include "horus_transmitter.h"
#include "ascent.h"
#include "4fsk.h"

//The Si4432 has the following connections:
//nSEL: 15
//nIRQ: 4
//SDN: 16

//Constructor for RadioLib radio
Si4432 radio = new Module(15,4,16);

#define TX_FREQ         434.600
#define FSK4_BAUD       100
#define FSK4_SPACING    624    // NOTE: This results in a shift of 312 or 624 due to PLL resolution of the Si4432. Resolution is 156Hz
#define TX_POWER        17 //dBm
#define RESET_PACKETS   1 //Reset radio every x packets

//Correction coeffecients

int16_t correction[4] = {0,0,0,0}; //Play around with these for the correction. The Si4432 does weird things sometimes ¯\_(ツ)_/¯

TinyGPSPlus gps;

// Init ESP8266 timer 1
ESP8266Timer ITimer;

volatile bool readGPS = false; //Are we reading the GPS?

//GPS commands

const char GPS_HAB[] = "$PCAS11,5*18\r\n"; //Set high altitude mode
const char GPS_DISABLE[] = "$PCAS04,1*18\r\n"; //Set GPS-only mode (no BeiDou)

int state; //radio state

int gps_counter = 0;

//FSK4Client fsk4(&radio);

void IRAM_ATTR TimerHandler(){ //Timer interrupt for reading GPS. IRAM_ATTR indicates that it is stored in RAM for quick execution
  gps_counter++;
  if (gps_counter % 40 == 0){
    Serial.write(GPS_HAB);
  }
  while (Serial.available() > 0){
    char NMEA_data = Serial.read();
    if(gps.encode(NMEA_data)){
      readGPS = true;
      if (gps.location.isValid()){
        //Quick intervention for ascent rate calculation: store current altitude and last altitude
        altitude_previous = altitude;

        gps_valid = true;
        latitude = gps.location.lat();
        longitude = gps.location.lng();
        altitude = gps.altitude.meters();
        time_hour = gps.time.hour();
        time_minute = gps.time.minute();
        time_second = gps.time.second();
        sats = gps.satellites.value();
        speed = gps.speed.knots();

        //For ascent rate calculation: if we have enough gps positions (counter > 2), call update rates

        if (packet_count > 2){
          updateRates();
        }

    }

    }
    
  }
}

void setupRadio(){
  SPI.pins(14,12,13,15); //Set the SPI pins
  SPI.begin(); //Begin SPI
  state = radio.begin(); //Begin radio
  while (state != RADIOLIB_ERR_NONE){
    state = radio.begin();
    Serial.println(state);
  }
  state = radio.setOutputPower(TX_POWER); //20dBm = 100mW, 17dBm = 50mw
  while (state != RADIOLIB_ERR_NONE){
    state = radio.setOutputPower(TX_POWER);
    Serial.println(state);
  }
  fsk4_setup(&radio, TX_FREQ, FSK4_SPACING, FSK4_BAUD);
  fsk4_correction(correction); //Apply correction
}


void setup() {
  Serial.begin(9600);
  pinMode(A0,INPUT);
  setupRadio();
  pinMode(2,OUTPUT);
  digitalWrite(2,HIGH); //LED off (inverted logic)

  //Start the GPS ISR
  ITimer.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, TimerHandler);

  beginSensors();

  pinMode(2,OUTPUT);
}

int counter_reset = 0;

void loop() {
  counter_reset++;
  if (counter_reset % RESET_PACKETS == 0){
    radio.reset();
    SPI.end(); //End SPI
    setupRadio(); //Prevent radio bricks and restart SPI bus
  }
  if (gps_valid){
    digitalWrite(2,LOW);
    readTemperatures(); //Read temperatures recorded from last time
    //Transmit packet
    // send some bytes as a preamble
    int pkt_len = build_horus_binary_packet_v2(rawbuffer);
    int coded_len = horus_l2_encode_tx_packet((unsigned char*)codedbuffer, (unsigned char*)rawbuffer, pkt_len);

    //Write preamble to ensure sync 

    requestTemperatures(); //First request temperatures for next time, then transmit

    fsk4_preamble(&radio, 8);
    fsk4_write(&radio, codedbuffer, coded_len);

    packet_count++;

  }
  else{
    //fsk4_preamble(&radio,256);
    if (readGPS){
      digitalWrite(2,HIGH);
      delay(250);
      digitalWrite(2,LOW);
      delay(250);

    }
    else{
      digitalWrite(2,HIGH);
      delay(1000);
      digitalWrite(2,LOW);
      delay(100);
    }
    
  }
  
}
