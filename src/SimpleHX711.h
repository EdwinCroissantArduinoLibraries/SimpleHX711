#ifndef SIMPLEHX711_H
#define SIMPLEHX711_H
#define SIMPLEHX711LIBVERSION "0.0.2"

/*
 * HX711 24-Bit Analog-to-Digital Converter for Bridge Sensors library
 * for the Arduino microcontroller.
 *
 * Copyright (C) 2016 Edwin Croissant
 *
 *  This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See the README.md file for additional information.
 * Revisions:
 * 08may2017 added getTimestamp and removed the 256 divisor in raw values
 */

#include "Arduino.h"

class SimpleHX711 {
public:
	enum gain {
		gain32 = 32,
		gain64 = 64,
		gain128 = 128
	};
	enum status {
		init, valid, poweredDown, timedOut
	};
	SimpleHX711(uint8_t pinClk, uint8_t pinData, byte readsUntilValid = 3, gain gain = gain128);
	bool read();
	status getStatus();
	void setGain(gain gain);
	gain getGain();
	void setAlpha(uint8_t alpha);
	uint8_t getAlpha();
	uint32_t getTimestamp();
	int32_t getRaw(bool smoothed = false);
	void tare(bool smoothed = false);
	void setTare(int32_t tare);
	int32_t getTare();
	int32_t getRawMinusTare(bool smoothed = false);
	void adjustTo(int32_t value, bool smoothed = false);
	int32_t getAdjuster();
	void setAdjuster(int32_t adjuster);
	int32_t getAdjusted(bool smoothed = false);
	void powerDown();
	void powerUp();
	void setReadsUntilValid(uint8_t readsUntilValid);
	uint8_t getReadsUntilValid();

private:
	uint8_t _pinClk;
	uint8_t _pinData;
	gain _gain;
	int32_t _tare;
	uint8_t _alpha;
	uint32_t _timestamp;
	int32_t _raw;
	int32_t _smoothedRaw;
	int32_t _adjuster;
	uint32_t _conversionStartTime;
	status _status;
	uint8_t _readCount;
	uint8_t _readsUntilValid;
	};

#endif //  SIMPLEHX711_H
