#include "SimpleHX711.h"


/*
 * Makes an instance of the library. The clock and data pins are required.
 * The readsUntilValid and gain are optional an default to 3 and 128 respectively
 * gain128 selects channel A and a gain of 128
 * gain64 selects channel A and a gain of 64
 * gain32 selects channel B and a gain of 32
 * After a reset there is a ~400 ms delay before the first reading with a gain
 * of 128 on channel A is ready. Stable output after changing the gain to 64
 * is reached after ~6 readings. readsUntilValid  sets the amount of successful
 * readings before the output is considered valid after a reset of the chip
 */

SimpleHX711::SimpleHX711(const uint8_t pinClk, const uint8_t pinData,
		byte readsUntilValid, SimpleHX711::gain gain) {
	pinMode(pinData, INPUT_PULLUP);
	pinMode(pinClk, OUTPUT);
	_pinClk = pinClk;
	_pinData = pinData;
	_gain = gain;
	_tare = 0;
	_raw = 0;
	_smoothedRaw = 0;
	_alpha = 200;
	_adjuster = 256;
	_conversionStartTime = millis();
	_readCount = 0;
	_status = init;
	_readsUntilValid = readsUntilValid;
	_timestamp = 0;
}

/*
 * returns true when done, poweredDown, timedOut
 * returns false when busy or readCount not reached
 */
bool SimpleHX711::read() {
	int8_t i, j;
	/*
	 * is the chip powered down?
	 */
	if (digitalRead(_pinClk)) {
		_status = poweredDown;
		return true;
	};

	/*
	 * is the chip still busy ?
	 */
	if (digitalRead(_pinData)) {
		/*
		 * the initializing time after powerup, reset and gain change
		 * is 400 ms when the output data rate is 10 Hz so return
		 * a time out after 500 ms
		 *
		 */
		if ((millis() - _conversionStartTime) >= 500) {
			_status = timedOut;
			return true;
		} else
			return false;
	};

	/*
	 * after a timedOut the scale must be initialized again
	 */
	if (_status == timedOut) {
		_status = init;
		_readCount = 0;
	}

	/*
	 * copy the conversion start time into the timestamp and
	 * read the 24 bits and put them in the MSB's of the 32 bit variable
	 * effectively multiplying it by 256
	 */
	_timestamp = _conversionStartTime;

	for (j = 3; j > 0; --j) {
		for (i = 0; i < 8; ++i) {
			digitalWrite(_pinClk, HIGH);
			reinterpret_cast<uint8_t*>(&_raw)[j] =
					(reinterpret_cast<uint8_t*>(&_raw)[j] << 1)
							+ digitalRead(_pinData);
			digitalWrite(_pinClk, LOW);
		}
	}
	/*
	 * additional clock cycles are required to set the gain and select the channel
	 */
	switch (_gain) {
	case gain64:
		/*
		 * three more clock cycles to select channel A and gain 64
		 */
		digitalWrite(_pinClk, HIGH);
		digitalWrite(_pinClk, LOW);
		/*
		 * this was only one clock cycle so no break as we want
		 * to fall through for the additional two clock cycles
		 */
	case gain32:
		/*
		 * two more clock cycles to select channel B and gain 32
		 */
		digitalWrite(_pinClk, HIGH);
		digitalWrite(_pinClk, LOW);
		/*
		 * this was only one clock cycle so no break as we want
		 * to fall through for the additional clock cycle
		 */
	default:
		/*
		 * one clock cycle to select channel A and gain 128
		 */
		digitalWrite(_pinClk, HIGH);
		digitalWrite(_pinClk, LOW);
	}
	/*
	 * save the time for timedOut
	 */
	_conversionStartTime = millis();
	/*
	 * the amount of reads before a stable output depends
	 * on the gain
	 */
	if (_readCount < _readsUntilValid) {
		++_readCount;
		if (_readCount < (_readsUntilValid))
			return false;
		else
			/*
			 * first valid read
			 */
			_smoothedRaw = _raw;
	} else
		/*
		 * exponential smoothing calculation
		 */
		_smoothedRaw += (_raw - _smoothedRaw) / 256 * _alpha;

	_status = valid;

	return true;
}

/*
 * the status of the last read
 * init : the chip is initializing and has not reached the readsUntilValid
 * valid : the last reading is valid
 * poweredDown : the chip is powered down
 * timedOut : the initializing time after powerup, reset and gain change
 * is 400 ms when the output data rate is 10 Hz, if the chip is not done
 * after 500 ms it's probably disconnected
 */
SimpleHX711::status SimpleHX711::getStatus() {
	return _status;
}

/*
 * possible values are gain128, gain64 (channel A) and gain32 (channel B)
 * expect up to 1400 ms delay before
 * valid output data is available
 */
void SimpleHX711::setGain(SimpleHX711::gain gain) {
	_gain = gain;
	_status = init;
	_readCount = 0;
}

/*
 * returns the gain
 */
SimpleHX711::gain SimpleHX711::getGain() {
	return _gain;
}

/*
 * alpha is the smoothing factor for the exponential smoothing
 * an input of 128 gives an alpha 128/ 256 = 0.5
 */
void SimpleHX711::setAlpha(uint8_t alpha) {
	_alpha = alpha;
}

/*
 * returns the value of alpha
 */
uint8_t SimpleHX711::getAlpha() {
	return _alpha;
}

/*
 * returns the timestamp in millis from the current reading
 * the timestamp is taken at the start of the conversion
 */

uint32_t SimpleHX711::getTimestamp() {
	return _timestamp;
}

/*
 * returns the raw 32 bit reading from the sensor
 * the boolean smoothed is optional and defaults to false
 */

int32_t SimpleHX711::getRaw(bool smoothed) {
	return smoothed ? _smoothedRaw : _raw;
}

/*
 * sets the tare to the raw reading
 * the boolean smoothed is optional and defaults to false
 */
void SimpleHX711::tare(bool smoothed) {
	_tare = smoothed ? _smoothedRaw : _raw;
}

/*
 * sets the value of the tare
 */
void SimpleHX711::setTare(int32_t tare) {
	_tare = tare;
}

/*
 * returns the value of the tare
 */
int32_t SimpleHX711::getTare() {
	return _tare;
}

/*
 * returns the raw 32 bits reading form the chip minus the tare
 * the boolean smoothed is optional and defaults to false
 */
int32_t SimpleHX711::getRawMinusTare(bool smoothed) {
	return smoothed ? (_smoothedRaw - _tare) : (_raw - _tare);
}

/*
 * sets the value returned by getAdjusted to the required value.
 * the boolean smoothed is optional, defaults to false
 * and defines if the raw reading or the smoothed reading
 * is used in the calculation of the adjuster
 */
void SimpleHX711::adjustTo(int32_t value, bool smoothed) {
	// prevent divide by zero
	if (!value)
		value = 1;
	_adjuster =
			smoothed ?
					(_smoothedRaw - _tare) / value :
					(_raw - _tare) / value;
}

/*
 * returns the value of the adjuster
 */
int32_t SimpleHX711::getAdjuster() {
	return _adjuster;
}

/*
 * sets the value of the adjuster
 */
void SimpleHX711::setAdjuster(int32_t adjuster) {
	_adjuster = adjuster;
}

/*
 * returns an adjusted reading, the boolean smoothed
 * is optional and defaults to false
 */
int32_t SimpleHX711::getAdjusted(bool smoothed) {
	return smoothed ?
			(_smoothedRaw - _tare) / _adjuster :
			(_raw - _tare) / _adjuster;
}

/*
 * bring chip in power down mode
 */
void SimpleHX711::powerDown() {
	digitalWrite(_pinClk, HIGH);
}

/*
 * powerUp will reset the chip.
 */
void SimpleHX711::powerUp() {
	digitalWrite(_pinClk, LOW);
	_status = init;
	_readCount = 0;
	// prevent timeout
	_conversionStartTime = millis();
}

/*
 * sets the amount of successful readings before the
 * output is considered valid after a reset of the chip
 */
void SimpleHX711::setReadsUntilValid(uint8_t readsUntilValid) {
		_readsUntilValid = readsUntilValid;
}

/*
 * returns the amount of successful readings before the
 * output is considered valid after a reset of the chip
 */
uint8_t SimpleHX711::getReadsUntilValid() {
	return _readsUntilValid;
}

