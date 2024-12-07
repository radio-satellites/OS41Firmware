#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include "timerconfig.h"
#include "ESP8266TimerInterrupt.h"
#include "crc16.h"
#include "horus_l2.h"
#include "horus_transmitter.h"
#include "4fsk.h"

//The Si4432 has the following connections:
//nSEL: 15
//nIRQ: 4
//SDN: 16

//Constructor for RadioLib radio
Si4432 radio = new Module(15,4,16);

#define TX_FREQ         434.600
#define FSK4_BAUD       100
#define FSK4_SPACING    624    // NOTE: This results in a shift of 312 due to PLL resolution of the Si4432. Resolution is 156Hz

//Correction coeffecients

int16_t correction[4] = {0,0,0,0}; //Play around with these for the correction. The Si4432 does weird things sometimes ¯\_(ツ)_/¯

TinyGPSPlus gps;

// Init ESP8266 timer 1
ESP8266Timer ITimer;

volatile bool readGPS = false; //Are we reading the GPS?

//GPS commands

const char GPS_HAB[] = "$PCAS11,5*18\r\n"; //Set high altitude mode
const char GPS_DISABLE[] = "$PCAS04,1*18\r\n"; //Set GPS-only mode (no BeiDou)


void IRAM_ATTR TimerHandler(){ //Timer interrupt for reading GPS. IRAM_ATTR indicates that it is stored in RAM for quick execution
  while (Serial.available() > 0){
    char NMEA_data = Serial.read();
    if(gps.encode(NMEA_data)){
      readGPS = true;
      if (gps.location.isValid()){
        gps_valid = true;
        latitude = gps.location.lat();
        longitude = gps.location.lng();
        altitude = gps.altitude.meters();
        time_hour = gps.time.hour();
        time_minute = gps.time.minute();
        time_second = gps.time.second();
        sats = gps.satellites.value();
        speed = gps.speed.knots();

    }

    }
    
  }
}


void setup() {
  Serial.begin(9600);
  SPI.pins(14,12,13,15); //Set the SPI pins
  SPI.begin(); //Begin SPI

  radio.begin(); //Begin radio
  radio.setOutputPower(10); //20dBm = 100mW, 17dBm = 50mw
  fsk4_setup(&radio, TX_FREQ, FSK4_SPACING, FSK4_BAUD);
  fsk4_correction(correction); //Apply correction
  pinMode(2,OUTPUT);
  digitalWrite(2,HIGH); //LED off (inverted logic)

  //Start the GPS ISR
  ITimer.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, TimerHandler);

  pinMode(2,OUTPUT);
}

void loop() {
  if (gps_valid){
    digitalWrite(2,LOW);
    //Transmit packet
    // send some bytes as a preamble
    int pkt_len = build_horus_binary_packet_v2(rawbuffer);
    int coded_len = horus_l2_encode_tx_packet((unsigned char*)codedbuffer, (unsigned char*)rawbuffer, pkt_len);

    //Write preamble to ensure sync 

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
