#include "stdint.h"
#include <cstring>

// Buffers and counters.
char rawbuffer [128];   // Buffer to temporarily store a raw binary packet.
char codedbuffer [128]; // Buffer to store an encoded binary packet
uint16_t packet_count = 0;  // Packet counter

//Variables for GPS reading

volatile bool gps_valid = false; //Should be false, true for testing radio (immediate transmit)
float latitude = 0.0;
float longitude = 0.0;
uint16_t altitude = 0;
uint8_t time_hour = 0;
uint8_t time_minute = 0;
uint8_t time_second = 0;
uint8_t sats = 0;
uint16_t speed = 0;


uint8_t convertToSondeHub(uint16_t adcReading){
  //This function takes in an input voltage and converts it to a range of 0-255 matching 0-5V to be sondehub compatible
  float inputVoltage = adcReading * (3.3 / 1023.0);
  float ratio = inputVoltage/5.0;
  uint8_t sondehubrange = round(ratio*255.0);
  return sondehubrange;
}

// Horus v2 Mode 1 (32-byte) Binary Packet
struct HorusBinaryPacketV2
{
    uint16_t     PayloadID;
    uint16_t	Counter;
    uint8_t	Hours;
    uint8_t	Minutes;
    uint8_t	Seconds;
    float	Latitude;
    float	Longitude;
    uint16_t  	Altitude;
    uint8_t     Speed;       // Speed in Knots (1-255 knots)
    uint8_t     Sats;
    int8_t      Temp;        // Twos Complement Temp value.
    uint8_t     BattVoltage; // 0 = 0v, 255 = 5.0V, linear steps in-between.
    // The following 9 bytes (up to the CRC) are user-customizable. The following just
    // provides an example of how they could be used.
    uint8_t     dummy1;      // unsigned int
    float     dummy2;       // Float 
    uint8_t     dummy3;     // battery voltage test
    uint8_t     dummy4;     // divide by 10
    uint16_t     dummy5;    // divide by 100
    uint16_t    Checksum;    // CRC16-CCITT Checksum.
}  __attribute__ ((packed));


int build_horus_binary_packet_v2(char *buffer){
  // Generate a Horus Binary v2 packet, and populate it with data

  struct HorusBinaryPacketV2 BinaryPacketV2;

  BinaryPacketV2.PayloadID = 604; // 604 = VE3SVF. Refer https://github.com/projecthorus/horusdemodlib/blob/master/payload_id_list.txt
  BinaryPacketV2.Counter = packet_count;
  BinaryPacketV2.Hours = time_hour;
  BinaryPacketV2.Minutes = time_minute;
  BinaryPacketV2.Seconds = time_second;
  BinaryPacketV2.Latitude = latitude;
  BinaryPacketV2.Longitude = longitude;
  BinaryPacketV2.Altitude = altitude;
  BinaryPacketV2.Speed = speed;
  BinaryPacketV2.BattVoltage = convertToSondeHub(analogRead(A0));
  BinaryPacketV2.Sats = sats;
  BinaryPacketV2.Temp = (int8)temperature;
  // Custom section. This is an example only, and the 9 bytes in this section can be used in other
  // ways. Refer here for details: https://github.com/projecthorus/horusdemodlib/wiki/5-Customising-a-Horus-Binary-v2-Packet
  BinaryPacketV2.dummy1 = 0;        // uint8
  BinaryPacketV2.dummy2 = 0.0;  // float32
  BinaryPacketV2.dummy3 = convertToSondeHub(analogRead(A0));      // uint8 - interpreted as a battery voltage 0-5V
  BinaryPacketV2.dummy4 = 0;      // uint8 - interpreted as a fixed-point value (div/10)
  BinaryPacketV2.dummy5 = 0;     // uint16 - interpreted as a fixed-point value (div/100)

  BinaryPacketV2.Checksum = (uint16_t)crc16((unsigned char *)&BinaryPacketV2, sizeof(BinaryPacketV2) - 2);


  memcpy(buffer, &BinaryPacketV2, sizeof(BinaryPacketV2));
	
  return sizeof(struct HorusBinaryPacketV2);
}