/*
 * MCP3564.cpp
 *
 * Simple Arduino/ESP32 driver for the Microchip MCP3561/MCP3562/MCP3564(R)
 * family of 24-bit Delta-Sigma ADCs (SPI interface).
 */

#include "MCP3564.h"

MCP3564::MCP3564(uint8_t csPin, uint8_t irqPin, SPIClass &spiBus, uint32_t spiHz)
  : _csPin(csPin),
    _irqPin(irqPin),
    _spi(spiBus),
    _spiSettings(spiHz, MSBFIRST, SPI_MODE0)
{
  pinMode(_csPin, OUTPUT);
  digitalWrite(_csPin, HIGH);
  
  // IRQ line from MCP3564 is active-low (pulls to GND when data is ready)
  pinMode(_irqPin, INPUT_PULLUP); 
}

void MCP3564::begin() {
  // 1. CONFIG0: Internal Osc, Active Conversion Mode (0x33)
  writeRegister8(REG_CONFIG0, 0x33);

  // 2. MUX: Set SCAN mode VNEG connection to AGND (Bits[3:0] = 0x0C)
  // This explicitly locks Vneg to AGND for all channels while scanning.
  writeRegister8(REG_MUX, 0x0C);

  // 3. SCAN: Enable channels CH0, CH1, CH2, and CH3 in the hardware scan list
  // Bits[23:16] map to the scanning sources. CH0-CH3 are bits 0 to 3 of that byte.
  // 0x0F0000 enables CH0, CH1, CH2, CH3.
  writeRegister24(REG_SCAN, 0x0F0000);

  // 4. TIMER: Set to 0 so the ADC advances to the next channel immediately 
  writeRegister24(REG_TIMER, 0x000000);
  
  // 5. Clear any residual IRQ state by reading CONFIG0
  readRegister8(REG_CONFIG0);
}

AdcResults MCP3564::readAllChannels() {
  AdcResults results;
  
  // Start the hardware sequential loop scan
  fastCommand(FASTCMD_CONV_START);

  // --- Channel 0 ---
  while (digitalRead(_irqPin) == HIGH); // Wait for physical IRQ pin to drop LOW
  results.ch0 = readRegister24(REG_ADCDATA); // Reading the data clears the IRQ state automatically

  // --- Channel 1 ---
  while (digitalRead(_irqPin) == HIGH);
  results.ch1 = readRegister24(REG_ADCDATA);

  // --- Channel 2 ---
  while (digitalRead(_irqPin) == HIGH);
  results.ch2 = readRegister24(REG_ADCDATA);

  // --- Channel 3 ---
  while (digitalRead(_irqPin) == HIGH);
  results.ch3 = readRegister24(REG_ADCDATA);

  return results;
}

// Helper: Assembles the 8-bit SPI instruction byte
uint8_t MCP3564::buildCommandByte(uint8_t addrOrFastCmd, uint8_t cmdType) {
  return ((DEVICE_ADDR & 0x03) << 6) | ((addrOrFastCmd & 0x0F) << 2) | (cmdType & 0x03);
}

void MCP3564::writeRegister8(uint8_t addr, uint8_t value) {
  uint8_t command = buildCommandByte(addr, CMDTYPE_INCR_WR);
  _spi.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  _spi.transfer(command);
  _spi.transfer(value);
  digitalWrite(_csPin, HIGH);
  _spi.endTransaction();
}

void MCP3564::writeRegister24(uint8_t addr, uint32_t value) {
  uint8_t command = buildCommandByte(addr, CMDTYPE_INCR_WR);
  _spi.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  _spi.transfer(command);
  _spi.transfer((value >> 16) & 0xFF);
  _spi.transfer((value >> 8) & 0xFF);
  _spi.transfer(value & 0xFF);
  digitalWrite(_csPin, HIGH);
  _spi.endTransaction();
}

uint8_t MCP3564::readRegister8(uint8_t addr) {
  uint8_t command = buildCommandByte(addr, CMDTYPE_STATIC_RD);
  uint8_t value = 0;
  _spi.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  _spi.transfer(command);
  value = _spi.transfer(0x00);
  digitalWrite(_csPin, HIGH);
  _spi.endTransaction();
  return value;
}

int32_t MCP3564::readRegister24(uint8_t addr) {
  uint8_t command = buildCommandByte(addr, CMDTYPE_STATIC_RD);
  int32_t rawData = 0;
  _spi.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  _spi.transfer(command);
  rawData |= ((uint32_t)_spi.transfer(0x00)) << 16;
  rawData |= ((uint32_t)_spi.transfer(0x00)) << 8;
  rawData |= ((uint32_t)_spi.transfer(0x00));
  digitalWrite(_csPin, HIGH);
  _spi.endTransaction();
  
  if (rawData & 0x800000) {
    rawData |= 0xFF000000;
  }
  return rawData;
}

void MCP3564::fastCommand(uint8_t fastCmd) {
  uint8_t command = buildCommandByte(fastCmd, CMDTYPE_FAST);
  _spi.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  _spi.transfer(command);
  digitalWrite(_csPin, HIGH);
  _spi.endTransaction();
}
}