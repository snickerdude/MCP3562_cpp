/*
 * MCP3564.cpp
 *
 * Simple Arduino/ESP32 driver for the Microchip MCP3561/MCP3562/MCP3564(R)
 * family of 24-bit Delta-Sigma ADCs (SPI interface).
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
    // Added irqPin parameter
    MCP3564(uint8_t csPin, uint8_t irqPin, SPIClass &spiBus = SPI, uint32_t spiHz = 4000000);
    
    // Configures pins, wakes the ADC, and programs SCAN mode with AGND as Vneg
    void begin();
    
    // Triggers a hardware scan of all configured channels and returns the packed struct
    AdcResults readAllChannels();

  private:
    uint8_t     _csPin;
    uint8_t     _irqPin; // 🆕 Track the IRQ hardware pin
    SPIClass    &_spi;
    SPISettings _spiSettings;

    static const uint8_t DEVICE_ADDR = 0x01;
    
    // Expanded register addresses for SCAN mode configuration
    static const uint8_t REG_ADCDATA = 0x0;
    static const uint8_t REG_CONFIG0 = 0x1;
    static const uint8_t REG_CONFIG1 = 0x2;
    static const uint8_t REG_IRQ     = 0x5;
    static const uint8_t REG_MUX     = 0x6;
    static const uint8_t REG_SCAN    = 0x7; // 🆕 Scan selection register
    static const uint8_t REG_TIMER   = 0x8; // 🆕 Scan delay timer register
 
    static const uint8_t FASTCMD_CONV_START = 0b1010;
 
    static const uint8_t CMDTYPE_FAST        = 0b00;
    static const uint8_t CMDTYPE_STATIC_RD  = 0b01;
    static const uint8_t CMDTYPE_INCR_WR    = 0b10;
    static const uint8_t CMDTYPE_INCR_RD    = 0b11;
 
    uint8_t buildCommandByte(uint8_t addrOrFastCmd, uint8_t cmdType);
    void    writeRegister8(uint8_t addr, uint8_t value);
    void    writeRegister24(uint8_t addr, uint32_t value);
    uint8_t readRegister8(uint8_t addr);
    int32_t readRegister24(uint8_t addr);
    void    fastCommand(uint8_t fastCmd);
};

#endif // MCP3564_H