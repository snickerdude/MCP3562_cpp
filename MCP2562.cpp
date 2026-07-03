/*
 * MCP3564.cpp
 *
 * Simple Arduino/ESP32 driver for the Microchip MCP3561/MCP3562/MCP3564(R)
 * family of 24-bit Delta-Sigma ADCs (SPI interface).
 */

#include "MCP3564.h"

MCP3564::MCP3564(uint8_t csPin, SPIClass &spiBus, uint32_t spiHz)
  : _csPin(csPin),
    _spi(spiBus),
    _spiSettings(spiHz, MSBFIRST, SPI_MODE0)
{
  pinMode(_csPin, OUTPUT);
  digitalWrite(_csPin, HIGH); // CS idles high (inactive)
}