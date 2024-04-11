/*
 * Copyright (c) 2024 Mika Laitio <lamikr@gmail.com>
 * 
 * This firmware is for the atmega168 located in i2c_to_toslink_pcb.
 * It's purpose is to initialize the cdcd906 so that it can generate the mclk clock
 * from other i2s clocks available. MCLK and other i2s clocks + datasignal are then feed
 * to wm8805 which will convert the i2s signal to optical toslink signal.
 *
 */

#include <Wire.h>

#define F_CPU 12000000UL
#define BAUD 12800UL

#define LED_PIN A0
#define I2C_ADDR__CDCE906 0x69
#define I2C_ADDR__WM8805  0x3A

static bool setup_ok;

bool i2c_read_uint8(int devaddr,
                      uint8_t regaddr,
                      uint8_t *data,
                      bool cdce906_single_byte_rw_operation)
{
  bool ret;
  int res;

  ret = false;
  // cdce906 single byte read and write operations requires that 
  // the bit 7 of registry is set to 1
  // and bulk read and write operations requires that the bit 7 is set to 0.
  if (cdce906_single_byte_rw_operation == true) {
    // cdce906 specific single byte read or write operation
    regaddr |= (1 << 7);
  }
  Wire.beginTransmission(devaddr);
  Wire.write(regaddr);
  Wire.endTransmission(false);  // do not send stop connection after sending requested address
  Wire.requestFrom(devaddr, 1); // read the registry
  *data = Wire.read();
  res = Wire.endTransmission(true);
  if (res == 0) {
    ret = true;
  }
  return ret;
}

bool i2c_write_uint8(int devaddr,
                    uint8_t regaddr,
                    uint8_t dataval,
                    bool cdce906_single_byte_rw_operation) {
  int res;
  bool ret;

  ret = false;
  // cdce906 single byte read and write operations requires that 
  // the bit 7 of registry is set to 1
  // and bulk read and write operations requires that the bit 7 is set to 0.
  if (cdce906_single_byte_rw_operation == true) {
    // cdce906 specific single byte read or write operation
    regaddr |= (1 << 7);
  }
  // Write a data register value
  Wire.beginTransmission(devaddr); // device
  Wire.write(regaddr); // register
  Wire.write(dataval); // data
  res = Wire.endTransmission(true);
  if (res == 0) {
    ret = true;
  }
  return ret;
}

bool cdce906_init(uint8_t devaddr) {
  int ii;
  byte res_int;
  bool res;
  uint8_t data;
  bool ret;

  ret = false;
  Wire.beginTransmission(devaddr);
  res_int = Wire.endTransmission();
  if (res_int == 0) {
    Serial.print("CDCE906 found, i2c addr: 0x");
    Serial.println(devaddr, HEX);
    // Set "Input Signal Source"
    delay(200);
    res = i2c_read_uint8(devaddr, 11, &data, true);
    if (res == true) {
      Serial.print("Old input signal source: 0x");
      Serial.println(data, HEX);
      data |= 1 << 6;
      data &= ~(1 << 7);
      Serial.print("New input signal source: 0x");
      Serial.println(data, HEX);
      res = i2c_write_uint8(devaddr, 11, data, true);
      if (res == true) {
        // Set Input Clock Selection (byte10, bit 4)
        res = i2c_read_uint8(devaddr, 10, &data, true);
        if (res == true) {
          // set clock source to CLK_IN1/BCLK
          data |= 1 << 4;    // Clock source = CLK_IN1 (BCLK input)
          i2c_write_uint8(devaddr, 10, data, true);
          // configure PLL to be 4x input signal
          // Y0 used for MCLK gen signal
          // cdcd906 datasheet page 18
          // Default setting of divider P0 = 10, P1 = 20, P2 = 8, P3 = 9, P4 = 32, and P5 = 4
          // P2 used by default for Y0, Y1, Y2, Y3, Y4 and Y5
          // cdcd906 datasheet page 27
          // sample_freq_out = (sample_freq_in x N) / (M x P2) = sample_freq_in x 32 / (1 x 8) = sample_freq_in * 4        
          i2c_write_uint8(devaddr, 1, 1, true);  // PLL1, M = 1
          res = i2c_write_uint8(devaddr, 2, 32, true); // PLL1, N = 32
          if (res == true) {
            ret = true;
          }
        }
      }
    }
    else {
      Serial.print("cdce906 failed to write input signal source");
    }
  }
  else {
    Serial.print("cdce906 not found response from in i2c addr: 0x");
    Serial.print(devaddr, HEX);
    Serial.print(", err: ");
    Serial.println(res_int);
  }
  delay(200);
  res = i2c_read_uint8(devaddr, 0, &data, true);
  if (res == true) {
    Serial.print("VENDOR_ID_AND_REV: ");
    Serial.print(data);
    Serial.println("");
    Serial.print("  Vendor_ID: ");
    Serial.print(data & 0x0F);
    Serial.println("");
    Serial.print("  Rev_code: ");
    Serial.println((data & 0xF0) >> 4);
  }
  else {
    Serial.print("Failed to read vendor id and revision");
  }
  return ret;
}

bool wm8805_init(uint8_t devaddr) {
  int res_int;
  uint8_t data;
  bool res;
  bool ret;

  ret = false;
  Wire.beginTransmission(devaddr);
  res_int = Wire.endTransmission();
  if (res_int == 0) {
    Serial.print("WM8805 found, i2c addr: 0x");
    Serial.println(devaddr, HEX);
    Serial.print("Device ID: ");  
    res = i2c_read_uint8(devaddr, 1, &data, false);
    if (data < 10) {
      Serial.print('0');
    } 
    Serial.print(data, HEX);
    res = i2c_read_uint8(devaddr, 0, &data, false);
    if (data < 10) {
      Serial.print('0');
    } 
    Serial.print(data, HEX);
    res = i2c_read_uint8(devaddr, 2, &data, false);
    if (res == true) {
      Serial.print(" Rev. ");  
      Serial.println(data, HEX);
      delay(2000);
      // resets, initializes and powers a wm8805 reset device
      res = i2c_write_uint8(devaddr, 0, 0, false);
      if (res == true) {
        // REGISTER 30
        // set the PWRDN
        // bit 7:6 - always 0
        // bit   5 - TRIOP      - Tri-state all Outputs => 0
        // bit   4 - AIFPD      - Digital Audio Interface Power Down => 0
        // bit   3 - OSCPD      - Oscillator Power Down => 0
        // bit   2 - SPDIFTXPD  - S/PDIF Transmitter Powerdown => 0
        // bit   1 - SPDIFRXPD  - S/PDIF Receiver Powerdown => 1 (off)
        // bit   0 - PLLPD      - PLL Powerdown => 0
        res = i2c_write_uint8(devaddr, 30, B00001011, false);
        if (res == true) {
          ret = true;
        }
      }
    }  
  }
  else {
    Serial.print("WM8896 not found response from in i2c addr: 0x");
    Serial.print(devaddr, HEX);
    Serial.print(", err: ");
    Serial.println(res_int);
  }
  return ret;
}

void setup() {
  bool res;

  setup_ok = false;
  Serial.begin(BAUD);
  while (!Serial);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Wire.begin();
  delay(5000);
  res = cdce906_init(I2C_ADDR__CDCE906);
  if (res == true) {
    delay(1000);
    res = wm8805_init(I2C_ADDR__WM8805);
    if (res == true) {
      setup_ok = true;
      Serial.println("i2c devices initialized ok");
      digitalWrite(LED_PIN, HIGH);
    }
  }
}

void loop() {
  // in error cases blink the led slowly
  delay(10000);
  if (setup_ok == false) {
    digitalWrite(LED_PIN, HIGH);
  }
  delay(1000);
  if (setup_ok == false) {
    digitalWrite(LED_PIN, LOW);
  }
}
