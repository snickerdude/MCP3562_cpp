/*
 * MCP3564.h
 *
 * Simple Arduino/ESP32 driver for the Microchip MCP3561/MCP3562/MCP3564(R)
 * family of 24-bit Delta-Sigma ADCs (SPI interface).
 *
 * Datasheet: DS20006391C
 */

#ifndef MCP3564_H
#define MCP3564_H

#include <Arduino.h>
#include <SPI.h>

struct AdcResults {
    int32_t ch0;
    int32_t ch1;
    int32_t ch2;
    int32_t ch3;
};

class MCP3564 {
  public:
    // csPin   = the microcontroller pin wired to the MCP3564's CS pin
    // spiBus  = which SPI bus to use (defaults to the board's primary SPI)
    // spiHz   = SPI clock speed in Hz (device supports up to 20 MHz)
    MCP3564(uint8_t csPin, SPIClass &spiBus = SPI, uint32_t spiHz = 4000000);
    
    // Simple begin function to call in the arduino setup() code.
    // Only sets the ADC oscillator source to internal.
    void begin();
    
    // Used to start an ADC conversion
    void startConversion();
    
    // Will return the data from the chose channel (0-3)
    // Single-ended conversions are measured against ground
    int32_t readChannel(uint8_t channel);

  private:
    uint8_t   _csPin;
    uint8_t   _irqPin;
    SPIClass  &_spi;
    SPISettings _spiSettings;

    // Device address bits (CMD[7:6]) hard-coded at the factory.
    // 0x01 is the standard default for these parts.
    static const uint8_t DEVICE_ADDR = 0x01;
    
    // Register addresses (Table 8-1)
    static const uint8_t REG_ADCDATA  = 0x0;
    static const uint8_t REG_CONFIG0  = 0x1;
    static const uint8_t REG_CONFIG1  = 0x2;
    static const uint8_t REG_CONFIG2  = 0x3;
    static const uint8_t REG_CONFIG3  = 0x4;
    static const uint8_t REG_IRQ      = 0x5;
    static const uint8_t REG_MUX      = 0x6;
    static const uint8_t REG_SCAN     = 0x7;
    static const uint8_t REG_TIMER    = 0x8;
 
    // Fast command codes (CMD[5:2] when CMD[1:0] = 00)
    static const uint8_t FASTCMD_CONV_START = 0b1010;
 
    // Command type codes (CMD[1:0])
    static const uint8_t CMDTYPE_FAST       = 0b00;
    static const uint8_t CMDTYPE_STATIC_RD  = 0b01;
    static const uint8_t CMDTYPE_INCR_WR    = 0b10;
    static const uint8_t CMDTYPE_INCR_RD    = 0b11;
    
    // MUX Constants for Negative input connection to AGND (0x0C)
    static const uint8_t MUX_AGND = 0x0C;
 
    uint8_t buildCommandByte(uint8_t addrOrFastCmd, uint8_t cmdType);
    void    writeRegister8(uint8_t addr, uint8_t value);
    uint8_t readRegister8(uint8_t addr);
    int32_t readRegister24(uint8_t addr);
    void    fastCommand(uint8_t fastCmd);
};

#endif // MCP3564_H