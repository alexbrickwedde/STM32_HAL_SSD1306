/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 by ThingPulse, Daniel Eichhorn
 * Copyright (c) 2018 by Fabrice Weinberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ThingPulse invests considerable time and money to develop these open source libraries.
 * Please support us by buying our products (and not the clones) from
 * https://thingpulse.com
 *
 */

#ifndef SSD1306Wire_h
#define SSD1306Wire_h

#include "OLEDDisplay.h"
#include <algorithm>

#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_STM32)
#define _min	min
#define _max	max
#endif
//--------------------------------------

class SSD1306Wire : public OLEDDisplay {
  private:
      uint8_t             _address;
      I2C_HandleTypeDef   *hi2c;

  public:

    /**
     * Create and initialize the Display using Wire library
     *
     * Beware for retro-compatibility default values are provided for all parameters see below.
     * Please note that if you don't wan't SD1306Wire to initialize and change frequency speed ot need to 
     * ensure -1 value are specified for all 3 parameters. This can be usefull to control TwoWire with multiple
     * device on the same bus.
     * 
     * @param _address I2C Display address
     * @param _sda I2C SDA pin number, default to -1 to skip Wire begin call
     * @param _scl I2C SCL pin number, default to -1 (only SDA = -1 is considered to skip Wire begin call)
     * @param g display geometry dafault to generic GEOMETRY_128_64, see OLEDDISPLAY_GEOMETRY definition for other options
     * @param _i2cBus on ESP32 with 2 I2C HW buses, I2C_ONE for 1st Bus, I2C_TWO fot 2nd bus, default I2C_ONE
     * @param _frequency for Frequency by default Let's use ~700khz if ESP8266 is in 160Mhz mode, this will be limited to ~400khz if the ESP8266 in 80Mhz mode
     */
    SSD1306Wire(uint8_t _address, I2C_HandleTypeDef *hi2c, OLEDDISPLAY_GEOMETRY g = GEOMETRY_128_64) {
      setGeometry(g);

      this->_address = _address << 1;
      this->hi2c = hi2c;
    }

    bool connect() {
      return true;
    }

    void display(void) {
      initI2cIfNeccesary();
      const int x_offset = (128 - this->width()) / 2;
      #ifdef OLEDDISPLAY_DOUBLE_BUFFER
        uint8_t minBoundY = UINT8_MAX;
        uint8_t maxBoundY = 0;

        uint8_t minBoundX = UINT8_MAX;
        uint8_t maxBoundX = 0;
        uint8_t x, y;

        // Calculate the Y bounding box of changes
        // and copy buffer[pos] to buffer_back[pos];
        for (y = 0; y < (this->height() / 8); y++) {
          for (x = 0; x < this->width(); x++) {
           uint16_t pos = x + y * this->width();
           if (buffer[pos] != buffer_back[pos]) {
             minBoundY = std::min(minBoundY, y);
             maxBoundY = std::max(maxBoundY, y);
             minBoundX = std::min(minBoundX, x);
             maxBoundX = std::max(maxBoundX, x);
           }
           buffer_back[pos] = buffer[pos];
         }
         yield();
        }

        // If the minBoundY wasn't updated
        // we can savely assume that buffer_back[pos] == buffer[pos]
        // holdes true for all values of pos

        if (minBoundY == UINT8_MAX) return;

        sendCommand(COLUMNADDR);
        sendCommand(x_offset + minBoundX);
        sendCommand(x_offset + maxBoundX);

        sendCommand(PAGEADDR);
        sendCommand(minBoundY);
        sendCommand(maxBoundY);

        uint8_t data[17];
        data[0] = 0x40;

        uint8_t k = 0;
        for (y = minBoundY; y <= maxBoundY; y++) {
          for (x = minBoundX; x <= maxBoundX; x++) {

            data[k + 1] = buffer[x + y * this->width()];
            k++;
            if (k == 16)  {
              HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, _address, data, k + 1, 100);
              if (ret != HAL_OK) {
//                asm("bkpt");
              }
              k = 0;
            }
          }
          yield();
        }

        if (k != 0) {
          HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, _address, data, k + 1, 100);
          if (ret != HAL_OK) {
//            asm("bkpt");
          }
        }
      #else

        sendCommand(COLUMNADDR);
        sendCommand(x_offset);
        sendCommand(x_offset + (this->width() - 1));

        sendCommand(PAGEADDR);
        sendCommand(0x0);

        for (uint16_t i=0; i < displayBufferSize; i++) {
          _wire->beginTransmission(this->_address);
          _wire->write(0x40);
          for (uint8_t x = 0; x < 16; x++) {
            _wire->write(buffer[i]);
            i++;
          }
          i--;
          _wire->endTransmission();
        }
      #endif
    }

    void setI2cAutoInit(bool doI2cAutoInit) {
    }

  private:
	int getBufferOffset(void) {
		return 0;
	}
    inline void sendCommand(uint8_t command) __attribute__((always_inline)){
      uint8_t data[2];
      data[0] = 0x80;
      data[1] = command;
      HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(hi2c, _address, data, 2, 100);
      if (ret != HAL_OK) {
//        asm("bkpt");
      }
    }

    void initI2cIfNeccesary() {
    }

};

#endif
