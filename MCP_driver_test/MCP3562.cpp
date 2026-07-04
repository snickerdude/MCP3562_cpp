/*
 * MCP3562.cpp
 *
 * Simple Arduino/ESP32 driver for the Microchip MCP3561/MCP3562/MCP3562(R)
 * family of 24-bit Delta-Sigma ADCs (SPI interface).
 */

#include "MCP3562.h"

MCP3562::MCP3562(uint8_t csPin, uint8_t irqPin, SPIClass &spiBus, uint32_t spiHz)
  : _csPin(csPin),
    _irqPin(irqPin),
    _spi(spiBus),
    _spiSettings(spiHz, MSBFIRST, SPI_MODE0)
{
  pinMode(_csPin, OUTPUT);
  digitalWrite(_csPin, HIGH);
  
  // IRQ line from MCP3562 is active-low (pulls to GND when data is ready)
  pinMode(_irqPin, INPUT_PULLUP); 
}

void MCP3562::begin() {
  // 1. CONFIG0: Internal Osc no output, Standby Mode (0xE3)
  writeRegister8(REG_CONFIG0, 0xE3);
  
	// CONFIG1: Reduce oversample ratio to increase samplerate
	writeRegister8(REG_CONFIG1, 0x04);
	
  // CONFIG3: One shot conversion to standby
  writeRegister8(REG_CONFIG3, 0x80);

	// hopefully disable SCAN mode
  writeRegister24(REG_SCAN, 0x000000); 
  
  // 5. Clear any residual IRQ state by reading CONFIG0
  readRegister8(REG_CONFIG0);
}

AdcResults MCP3562::readAllChannelsMux() {
  AdcResults results;
  
	// disable scan mode in preperation for a MUX read. 
	writeRegister8(REG_SCAN, 0x000000); 
	
  // --- Channel 0 ---
  writeRegister8(REG_MUX, MUX_CH0 | MUX_AGND); // Set MUX to CH0 vs AGND
  fastCommand(FASTCMD_CONV_START);
  while (digitalRead(_irqPin) == HIGH);
  results.ch0 = readRegister24(REG_ADCDATA); // Reading the data clears the IRQ state automatically

  // --- Channel 1 ---
  writeRegister8(REG_MUX, MUX_CH1 | MUX_AGND); // Set MUX to CH1 vs AGND
  fastCommand(FASTCMD_CONV_START);
  //startTime = millis();
  while (digitalRead(_irqPin) == HIGH);
  results.ch1 = readRegister24(REG_ADCDATA);

  // --- Channel 2 ---
  writeRegister8(REG_MUX, MUX_CH2 | MUX_AGND); // Set MUX to CH2 vs AGND
  fastCommand(FASTCMD_CONV_START);
  //startTime = millis();
  while (digitalRead(_irqPin) == HIGH);
  results.ch2 = readRegister24(REG_ADCDATA);

  // --- Channel 3 ---
  writeRegister8(REG_MUX, MUX_CH3 | MUX_AGND); // Set MUX to CH3 vs AGND
  fastCommand(FASTCMD_CONV_START);
  //startTime = millis();
  while (digitalRead(_irqPin) == HIGH);
  results.ch3 = readRegister24(REG_ADCDATA);

  return results;
}

AdcResults MCP3562::readAllChannelsScan() {
	AdcResults results;
	
	// Enable SCAN mode, 0 delay, channels 0-3 referenced to AGND
	writeRegister8(REG_SCAN, 0x0000F);
	
	// Begin the conversion
	fastCommand(FASTCMD_CONV_START);
	
	// Wait for conversion to finish
	while (digitalRead(_irqPin) == HIGH);
	
	// Execute an incremental read command
	uint8_t command = buildCommandByte(REG_ADCDATA, CMDTYPE_INCR_RD);
  
  _spi.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  
  _spi.transfer(command);
	// --- Channel 0 (3 bytes) ---
  int32_t raw0 = 0;
  raw0 |= ((uint32_t)_spi.transfer(0x00)) << 16;
  raw0 |= ((uint32_t)_spi.transfer(0x00)) << 8;
  raw0 |= ((uint32_t)_spi.transfer(0x00));
  if (raw0 & 0x800000) raw0 |= 0xFF000000;
  results.ch0 = raw0;

  // --- Channel 1 (3 bytes) ---
  int32_t raw1 = 0;
  raw1 |= ((uint32_t)_spi.transfer(0x00)) << 16;
  raw1 |= ((uint32_t)_spi.transfer(0x00)) << 8;
  raw1 |= ((uint32_t)_spi.transfer(0x00));
  if (raw1 & 0x800000) raw1 |= 0xFF000000;
  results.ch1 = raw1;

  // --- Channel 2 (3 bytes) ---
  int32_t raw2 = 0;
  raw2 |= ((uint32_t)_spi.transfer(0x00)) << 16;
  raw2 |= ((uint32_t)_spi.transfer(0x00)) << 8;
  raw2 |= ((uint32_t)_spi.transfer(0x00));
  if (raw2 & 0x800000) raw2 |= 0xFF000000;
  results.ch2 = raw2;

  // --- Channel 3 (3 bytes) ---
  int32_t raw3 = 0;
  raw3 |= ((uint32_t)_spi.transfer(0x00)) << 16;
  raw3 |= ((uint32_t)_spi.transfer(0x00)) << 8;
  raw3 |= ((uint32_t)_spi.transfer(0x00));
  if (raw3 & 0x800000) raw3 |= 0xFF000000;
  results.ch3 = raw3;

  digitalWrite(_csPin, HIGH);
  _spi.endTransaction();
  
  return results;
}

// Helper: Assembles the 8-bit SPI instruction byte
uint8_t MCP3562::buildCommandByte(uint8_t addrOrFastCmd, uint8_t cmdType) {
  return ((DEVICE_ADDR & 0x03) << 6) | ((addrOrFastCmd & 0x0F) << 2) | (cmdType & 0x03);
}

void MCP3562::writeRegister8(uint8_t addr, uint8_t value) {
  uint8_t command = buildCommandByte(addr, CMDTYPE_INCR_WR);
  _spi.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  _spi.transfer(command);
  _spi.transfer(value);
  digitalWrite(_csPin, HIGH);
  _spi.endTransaction();
}

void MCP3562::writeRegister24(uint8_t addr, uint32_t value) {
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

uint8_t MCP3562::readRegister8(uint8_t addr) {
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

int32_t MCP3562::readRegister24(uint8_t addr) {
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

void MCP3562::fastCommand(uint8_t fastCmd) {
  uint8_t command = buildCommandByte(fastCmd, CMDTYPE_FAST);
  _spi.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  _spi.transfer(command);
  digitalWrite(_csPin, HIGH);
  _spi.endTransaction();
}
