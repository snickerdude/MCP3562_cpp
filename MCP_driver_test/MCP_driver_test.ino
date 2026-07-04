#include <SPI.h>
#include "MCP3562.h"

#define ADC_CS_PIN  4 
#define ADC_IRQ_PIN 5  // Connect to the IRQ/MDAT pin of MCP3564
#define SPI_SPEED 15000000 // 20MHz SPi operation

// Instantiate with CS and IRQ pins
MCP3562 adc(ADC_CS_PIN, ADC_IRQ_PIN, SPI, SPI_SPEED);

void setup() {
  Serial.begin(115200);
  SPI.begin();
  
  Serial.println("Initializing MCP3562 in SCAN mode...");
  adc.begin();
  Serial.println("ADC Configured.");
}

void loop() {
  // Triggers the hardware to sweep CH0-CH3 against AGND
  int startcount = 0;
  int startcounttime = millis();
  Serial.println("Starting");
  /*
  while (startcount < 10000){
    AdcResults rslt = adc.readAllChannelsScan();
    startcount ++;
  }
  Serial.println(millis() - startcounttime);
  */
  
  AdcResults rslt = adc.readAllChannelsScan();
  
  Serial.println("--- ADC Readings (Hardware Sync) ---");
  Serial.print("CH0: "); Serial.println(rslt.ch0);
  Serial.print("CH1: "); Serial.println(rslt.ch1);
  Serial.print("CH2: "); Serial.println(rslt.ch2);
  Serial.print("CH3: "); Serial.println(rslt.ch3);
  
  //10000010010101011011111
  
  delay(1000); 
}