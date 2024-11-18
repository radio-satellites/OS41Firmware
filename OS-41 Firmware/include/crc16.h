#pragma once

#include <stdint.h>

uint16_t crc_xmodem_update(uint16_t crc, uint8_t data);
unsigned int crc16(unsigned char *string, unsigned int len);