#include <Wire.h>

#include "RTCx.h"

#define SECS_PER_DAY 86400L
#define SECS_PER_4_YEARS (SECS_PER_DAY * (366L + 365L + 365L + 365L))

#ifdef ARDUINO_ARCH_AVR
#define SNPRINTF snprintf_P
#else
#define SNPRINTF snprintf
#endif

// Day of week calculation needs to know the starting condition
#if (RTCX_EPOCH - 1970) % 28 == 0
const uint8_t RTCx::epochDow = 4;  // Thursday
#elif (RTCX_EPOCH - 1970) % 28 == 4
const uint8_t RTCx::epochDow = 2;  // Tuesday
#elif (RTCX_EPOCH - 1970) % 28 == 8
const uint8_t RTCx::epochDow = 0;  // Sunday
#elif (RTCX_EPOCH - 1970) % 28 == 12
const uint8_t RTCx::epochDow = 5;  // Friday
#elif (RTCX_EPOCH - 1970) % 28 == 16
const uint8_t RTCx::epochDow = 3;  // Wednesday
#elif (RTCX_EPOCH - 1970) % 28 == 20
const uint8_t RTCx::epochDow = 1;  // Monday
#elif (RTCX_EPOCH - 1970) % 28 == 24
const uint8_t RTCx::epochDow = 6;  // Saturday
#else
#error epochDow not defined
#endif


const uint8_t RTCx::DS1307Address = 0x68;
const uint8_t RTCx::MCP7941xAddress = 0x6F;
const uint8_t RTCx::MCP7941xEepromAddress = 0x57;
const uint8_t RTCx::PCF85263Address = 0x51;

const char RTCx::DS1307Str[] PROGMEM = "DS1307";
const char RTCx::MCP7941xStr[] PROGMEM = "MCP7941x";
const char RTCx::PCF85263Str[] PROGMEM = "PCF85263";
// Device names must be ordered according to their device_t enum value.
PGM_P const RTCx::deviceNames[] PROGMEM = {RTCx::DS1307Str, RTCx::MCP7941xStr, RTCx::PCF85263Str};

// The addresses used by the DS1307 and MCP7941x are also used by
// other devices (eg MCP3424 ADC) so test for these last.
const RTCx::device_t RTCx::autoprobeDeviceList[RTCX_NUM_SUPPORTED_DEVICES] = {
	PCF85263,
	DS1307,
	MCP7941x,
};
const uint8_t RTCx::autoprobeDeviceAddresses[RTCX_NUM_SUPPORTED_DEVICES] = {
	PCF85263Address,
	DS1307Address,
	MCP7941xAddress,
};


RTCx rtc;

bool RTCx::isLeapYear(uint16_t year)
{
	// Since year is > 1900 and < 2100 can use simple method
	return (year % 4) == 0;
}

uint8_t RTCx::daysInMonth(uint16_t year, uint8_t month)
{
	uint8_t daysInMonth[13] =
		{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (month == 2 && isLeapYear(year))
		return 29;
	return daysInMonth[month];
}

uint16_t RTCx::dayOfYear(uint16_t year, uint8_t month, uint8_t day)
{
	uint16_t doy = 0;
	uint8_t m = 1;
	while (m < month)
		doy += daysInMonth(year, m++);
	doy += day;
	return doy;
}

RTCx::time_t RTCx::mktime(struct tm *tm)
{
	// Normalise the time
	tm->tm_min += (tm->tm_sec / 60);
	tm->tm_sec = (tm->tm_sec % 60);
	if (tm->tm_sec < 0) {
		tm->tm_sec += 60;
		--(tm->tm_min);
	}

	tm->tm_hour += (tm->tm_min / 60);
	tm->tm_min = (tm->tm_min % 60);
	if (tm->tm_min < 0) {
		tm->tm_min += 60;
		--(tm->tm_hour);
	}

	tm->tm_mday += (tm->tm_hour / 24);
	tm->tm_hour = (tm->tm_hour % 24);
	if (tm->tm_hour < 0) {
		tm->tm_hour += 24;
		--(tm->tm_mday);
	}

	if (tm->tm_mon < 0 || tm->tm_mon > 11 || tm->tm_mday < 1)
		return -1;

	// Normalise the date
	while (true) {
		uint8_t dim = daysInMonth(tm->tm_year+1900, tm->tm_mon+1);
		if (tm->tm_mday > dim) {
			tm->tm_mday -= dim;
			++(tm->tm_mon);
			if (tm->tm_mon == 12) {
				tm->tm_mon = 0;
				++(tm->tm_year);
			}
			continue;
		}

		if (tm->tm_mday < 1) {
			--(tm->tm_mon);
			if (tm->tm_mon == -1) {
				tm->tm_mon = 11;
				--(tm->tm_year);
			}

			uint8_t dim = daysInMonth(tm->tm_year+1900, tm->tm_mon+1);
			tm->tm_mday += dim;
			continue;
		}
		break;
	}

	// Compute day of year
	tm->tm_yday = dayOfYear(tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday) - 1;

	uint8_t yearsSinceEpoch = tm->tm_year + 1900  - RTCX_EPOCH;
	time_t t = (yearsSinceEpoch * 365 * SECS_PER_DAY) // Whole years, leap days excluded
		+ ((yearsSinceEpoch / 4) * SECS_PER_DAY)  // Leap days in whole 4 year period
		// Leap days in partial 4 year period. Count only if in last year
		+ ((yearsSinceEpoch % 4) == 3 ? SECS_PER_DAY : 0L)
		+ (tm->tm_yday * SECS_PER_DAY)           // Whole days in current year
		+ (tm->tm_hour * 3600L)
		+ (tm->tm_min * (uint16_t)60)
		+ tm->tm_sec;

	// Compute day of week
	uint32_t daysSinceEpoch = (t / SECS_PER_DAY);
	tm->tm_wday = (daysSinceEpoch + epochDow) % 7; // 1970-01-01 was Thursday (day 4)
	return t;
}

struct RTCx::tm *RTCx::gmtime_r(const time_t *timep, struct tm *result)
{
	time_t t = *timep;
	// Find multiples of 4 years since epoch
	int8_t fourYears = (int8_t)(t / SECS_PER_4_YEARS);
	if (t < 0)
		--fourYears; // Now remaining time will be positive and must add
	result->tm_year = (fourYears * 4) + (RTCX_EPOCH - 1900); // years since 1900
	t -= (fourYears * SECS_PER_4_YEARS);

	// Split t into seconds in day and days remaining.
	int16_t days = (t / SECS_PER_DAY); // Fits into 16 bits
	time_t partialDay_s = (t % SECS_PER_DAY); // seconds

	// Calculate hours, minutes and seconds next so that the rest of the
	// calculations can be made in days using 16 bit arithmetic.
	result->tm_sec = (partialDay_s % 60);
	int16_t partialDay_m = partialDay_s / 60; // minutes
	result->tm_min = (partialDay_m % 60);
	result->tm_hour = partialDay_m / 60;

	if (days >= (365 + 365 + 366)) {
		// Third year in a four year block is a leap year
		days -= (365 + 365 + 366);
		result->tm_year += 3;
	}
	else
		while (days >= 365) {
			days -= 365;
			++(result->tm_year);
		}

	// days is now the day of year
	result->tm_yday = days;
	result->tm_mon = 0;
	result->tm_mday = 1 + days;

	while (true) {
		uint8_t dim = daysInMonth(result->tm_year+1900, result->tm_mon+1);
		if (result->tm_mday > dim) {
			result->tm_mday -= dim;
			++(result->tm_mon);
		}
		else
			break;
	}

	// Compute day of week
	uint16_t daysSinceEpoch = (*timep / 86400L);
	result->tm_wday = (daysSinceEpoch + epochDow) % 7; // 1970-01-01 was Thursday (day 4)
	return result;
}


RTCx::RTCx(void) : address(RTCx::DS1307Address), device(RTCx::DS1307)
{
	;
}

// Determine probable device from address
RTCx::RTCx(uint8_t a)
	: address(a)
{
	switch (address) {
	case RTCx::DS1307Address:
		device = RTCx::DS1307;
		break;
	case RTCx::MCP7941xAddress:
		device = RTCx::MCP7941x;
		break;
	default:
		// Guess DS1307
		device = RTCx::DS1307;
		break;
	}
}

RTCx::RTCx(uint8_t a, device_t d)
	: address(a), device(d)
{
	;
}

/* Autoprobe for a real-time clock, given a list of addresses to
 * check. Set the register address to zero before attempting to read a
 * byte; the MCP7941x will return a NACK if the register address is
 * not within its valid range.
 */
bool RTCx::autoprobe(const uint8_t *addressList, uint8_t len)
{
	for (uint8_t i = 0; i < len; ++i) {
		// Ensure register address is valid
		Wire.beginTransmission(addressList[i]);
		Wire.write(uint8_t(0));
		Wire.endTransmission();

		// Try reading a byte
		Wire.requestFrom(addressList[i], (uint8_t)1);
		if (Wire.available()) {
			*this = RTCx(addressList[i]);
			return true;
		}
	}
	return false;
}


/* Autoprobe for a real-time clock, given a list of devices to
 * check. Set the register address to zero before attempting to read a
 * byte; the MCP7941x will return a NACK if the register address is
 * not within its valid range.
 */
bool RTCx::autoprobe(const device_t *deviceList, const uint8_t *addressList, uint8_t len)
{
	for (uint8_t i = 0; i < len; ++i) {
		// Ensure register address is valid
		Wire.beginTransmission(addressList[i]);
		Wire.write(uint8_t(0));
		Wire.endTransmission();

		// Try reading a byte
		Wire.requestFrom(addressList[i], (uint8_t)1);
		if (Wire.available()) {
			*this = RTCx(addressList[i], deviceList[i]);
			return true;
		}
	}
	return false;
}

// Do whatever is needed for 'normal' operation. For the PCF85263 this
// means behaviour similar to the default DS1307 behaviour.
void RTCx::init(void) const
{
	if (device == PCF85263) {
		// Set various sensible defaults, including enabling battery
		// backup and zeroing the calibration.
		Wire.beginTransmission(address);
		Wire.write((uint8_t)0x23); // Start register
		Wire.write((uint8_t)0x00); // 0x23
		Wire.write((uint8_t)0x00); // 0x24 Two's complement offset value
		Wire.write((uint8_t)0x12); // 0x25 Normal offset correction, enable low-jitter mode, set load caps to 12.5pF
		Wire.write((uint8_t)0x00); // 0x26 Battery switch reg, same as after a reset
		Wire.write((uint8_t)0x00); // 0x27 Enable CLK pin, using bits set in reg 0x28
		Wire.write((uint8_t)0x07); // 0x28 Realtime clock mode, no periodic interrupts, CLK pin off
		Wire.write((uint8_t)0x00); // 0x29
		Wire.write((uint8_t)0x00); // 0x2a
		Wire.endTransmission();
	}
	else {
		enableBatteryBackup();
		setCalibration(0);
	}

	startClock();
}

bool RTCx::resetClock(void) const
{
	switch (device) {
	case PCF85263:
		writeData(0x2f, 0x2c);
		return true;

	case DS1307:
	case MCP7941x:
	default:
		// Nothing to do
		return false;
	}
}

void RTCx::stopClock(void) const
{
	if (device == PCF85263) {
		writeData(0x2e, 1);
		return;
	}

	uint8_t reg = 0;
	uint8_t s = readData(reg);
	switch (device) {
	case DS1307:
		s |= 0x80;
		break;
	case MCP7941x:
		s &= 0x7f;
		break;
	case PCF85263:
		break;
	}
	writeData(reg, s);
}

// Start the clock. If bcdSec is >= 0 zero use its value for the
// seconds instead of reading the current value. This helps reduce the
// number of I2C reads and writes required.
void RTCx::startClock(int16_t bcdSec) const
{
	if (device == PCF85263) {
		writeData(0x2e, 0);
		return;
	}

	uint8_t s;
	uint8_t reg = 0;
	if (bcdSec < 0)
		s = readData(reg);
	else
		s = uint8_t(bcdSec & 0x7f);

	uint8_t s2 = s;
	switch (device) {
	case MCP7941x:
		s2 |= 0x80; // Enable start bit
		break;
	case DS1307:
		s2 &= 0x7f; // Clear clock halt
		break;
	case PCF85263:
		break;
	}

	// Write back the data if it is different to the contents of the
	// register.  Always write back if the data wasn't fetched with
	// readData as the contents of the stop bit are unknown.
	if (s != s2 || bcdSec < 0)
		writeData(reg, s2);
}


/* Read a time from the clock. The same function is also used to read
 * the alarms as the register layout is essentially identical but with
 * week day and year omitted.
 */
bool RTCx::readClock(struct tm *tm, timeFunc_t func) const
{
	// Find which register to read from
	uint8_t sz = 0;
	uint8_t reg = getRegister(func, sz);

	if (sz == 0)
		return false; // not supported

	if (device == MCP7941x &&
		(func == TIME_POWER_FAILED || func == TIME_POWER_RESTORED))
		return readTimeSaver(tm, reg, sz);

	while (true) {
		// Reset the register pointer
		Wire.beginTransmission(address);
		Wire.write(reg);
		Wire.endTransmission();

		Wire.requestFrom(address, sz);
		tm->tm_sec = bcdToDec(Wire.read() & 0x7f);
		tm->tm_min = bcdToDec(Wire.read() & 0x7f);
		uint8_t h = Wire.read();
		if (h & 0x40) {
			// Twelve hour mode
			tm->tm_hour = bcdToDec(h & 0x1f);
			if (h & 0x20)
				tm->tm_hour += 12; // Seems notation for AM/PM is user-defined
		}
		else
			tm->tm_hour = bcdToDec(h & 0x3f);

		if (device == PCF85263) {
			// Day of month is before day of week!
			tm->tm_mday = bcdToDec(Wire.read() & 0x3f);
			tm->tm_wday = (Wire.read() & 0x07); // Clock uses [0..6]
		}
		else {
			tm->tm_wday = (Wire.read() & 0x07) - 1; // Clock uses [1..7]
			tm->tm_mday = bcdToDec(Wire.read() & 0x3f);
		}

		tm->tm_mon = bcdToDec(Wire.read() & 0x1f) - 1; // Clock uses [1..12]
		if (sz >= 7)
			tm->tm_year = bcdToDec(Wire.read()) + 100; // Assume 21st century
		else
			tm->tm_year = (RTCX_EPOCH - 1900);
		tm->tm_yday = -1;
		Wire.endTransmission();

		if ((func != TIME) || (tm->tm_sec == bcdToDec(readData(reg) & 0x7f)))
			break;
	}
	return true;
}


bool RTCx::readClock(char* buffer, size_t len, timeFunc_t func) const
{
	// YYYY-MM-DDTHH:MM:SS
	// 12345678901234567890
	if (buffer == NULL || len < 20)
		return false;
	struct tm tm;
	if (!readClock(tm, func))
		return false;
	int n = isotime(tm, buffer, len);
	return size_t(n) < len; // If n >= len the string was truncated
}


bool RTCx::setClock(const struct tm *tm, timeFunc_t func) const
{
	// Find which register to read from
	uint8_t sz = 0;
	uint8_t reg = getRegister(func, sz);

	if (sz == 0)
		return false; // Function not supported

	uint8_t clockHalt = 0;
	uint8_t osconEtc = 0;

	if (func == TIME) {
		if (device == PCF85263) {
			// Stop clock and clear prescaler in one operation
			Wire.beginTransmission(address);
			Wire.write((uint8_t)0x2e);
			Wire.write((uint8_t)1); // 0x2e Stop the clock
			Wire.write((uint8_t)0xa4); // 0x2f STOP
			// Register wraps round to 0x00
			Wire.write((uint8_t)0); // Clear hundredths of seconds
			// Now ready to write seconds
			// Wire.endTransmission();
		}
		else {
			stopClock();

			if (device == DS1307)
				clockHalt = 0x80; // Clock halt to be kept enabled for now

			if (device == MCP7941x)
				// Preserve OSCON, VBAT, VBATEN on MCP7941x
				osconEtc = readData((uint8_t)0x03) & 0x38;

			Wire.beginTransmission(address);
			Wire.write(reg);
			// Now ready to write seconds
		}
	}

	// Wire.beginTransmission(address);
	// Wire.write(reg);
	Wire.write(decToBcd(tm->tm_sec) | clockHalt);

	Wire.write(decToBcd(tm->tm_min));
	Wire.write(decToBcd(tm->tm_hour)); // Forces 24h mode

	if (device == PCF85263) {
		// Day of month and weekday are in reverse order compared to
		// DS1307 and MCP7941x
		Wire.write(decToBcd(tm->tm_mday));
		Wire.write(decToBcd(tm->tm_wday)); // Clock uses [0..6]
	}
	else {
		Wire.write(decToBcd(tm->tm_wday + 1) | osconEtc); // Clock uses [1..7]
		Wire.write(decToBcd(tm->tm_mday));
	}

	// Leap year bit on MCP7941x is read-only so ignore it
	Wire.write(decToBcd(tm->tm_mon + 1));

	if (sz >= 7)
		Wire.write(decToBcd(tm->tm_year % 100));

	Wire.endTransmission();

	if (func == TIME)
		// startClock(decToBcd(tm->tm_sec));
		startClock();
	return true;
}


bool RTCx::setClock(const char* s, timeFunc_t func) const
{
	if (s == NULL || strlen(s) < 19)
		return false;
	struct tm tm;
	tm.tm_year = atoi(s) - 1900;
	s += 5;
	tm.tm_mon = atoi(s) - 1;
	s += 3;
	tm.tm_mday = atoi(s);
	s += 3;
	tm.tm_hour = atoi(s);
	s += 3;
	tm.tm_min = atoi(s);
	s += 3;
	tm.tm_sec = atoi(s);
	mktime(&tm);
	return setClock(&tm, func);
}

bool RTCx::adjustClock(RTCx::time_t offset) const
{
	struct tm tm;
	if (!readClock(&tm))
		return false;
	time_t now = mktime(&tm);
	now -= offset;
	gmtime_r(&now, &tm);
	return setClock(&tm);
}


bool RTCx::setSQW(freq_t f) const
{
	switch (device) {
	case DS1307:
		if (f <= freq32768Hz) {
			writeData(0x07, uint8_t(0x10) | (f & uint8_t(0x03)));
			return true;
		}
		break;
	case MCP7941x:
		if (f <= freqCalibration) {
			uint8_t ctrl = readData(0x07) & uint8_t(0xf8);
			ctrl |= f;
			ctrl |=0x40; // Enable square wave
			writeData(0x07, ctrl);
			return true;
		}
		break;
	case PCF85263:
		uint8_t val;
		switch (f) {
		case freq32768Hz:
			val = 0;
			break;
		case freq16384Hz:
			val = 1;
			break;
		case freq8192Hz:
			val = 2;
			break;
		case freq4096Hz:
			val = 3;
			break;
		case freq2048Hz:
			val = 4;
			break;
		case freq1024Hz:
			val = 5;
			break;
		case freq1Hz:
			val = 6;
			break;
		case freqOutputLow:
			val = 7;
			break;
		default:
			return false;
		}
		writeData(0x28, val);
		// Ensure CLK pin is enabled
		uint8_t pinIoRegVal = readData(0x27);
		pinIoRegVal &= 0x7f;
		writeData(0x27, pinIoRegVal);
		return true;
	}
	return false;
}


void RTCx::enableBatteryBackup(bool enable) const
{
	if (device == MCP7941x) {
		// Writing to register 0x03 will clear the power-fail flag and
		// zero the power fail and power restored timestamps. Only
		// actually enable the bit if it is not already set.
		if (bool(readData(0x03) & 0x08) == enable)
			// State matches that requested
			return;

		stopClock();
		uint8_t d = readData(0x03);
		if (enable)
			d |= 0x08;
		else
			d &= 0xf7;
		writeData((uint8_t)0x03, d);
		startClock();
	}
}


bool RTCx::getPowerFailFlag(void) const
{
	if (device == MCP7941x)
		return readData(0x03) & 0x10;

	return false;
}


void RTCx::clearPowerFailFlag(void) const
{
	if (device == MCP7941x) {
		stopClock();
		uint8_t d = readData((uint8_t)0x03);
		d &= 0xef;
		writeData((uint8_t)0x03, d);
		startClock();
	}
}


int8_t RTCx::getCalibration(void) const
{
	switch (device) {
	case MCP7941x:
		{
			// Convert from signed magnitude to two's complement.
			uint8_t d = readData(0x08);
			int8_t r = d & 0x7Fu;
			return ((d & 0x80u) ? -r : r);
		}
		break;

	case PCF85263:
		return readData(0x24);
		break;

	case DS1307:
		break;
	}
	return 0;
}


bool RTCx::setCalibration(int8_t cal) const
{
	switch (device) {
	case MCP7941x:
		// Convert two's complement to signed magnitude.
		if (cal == -128)
			cal = -127; // Out of range, use next best value.
		uint8_t d;
		if (cal < 0)
			d = 0x80u | (uint8_t)(-cal);
		else
			d = cal;
		writeData(0x08, d);
		return true;

	case PCF85263:
		writeData(0x24, cal);
		return true;

	case DS1307:
		break;
	}
	return false;
}


uint8_t RTCx::bcdToDec(uint8_t b)
{
	return ( ((b >> 4)*10) + (b%16) );
}


uint8_t RTCx::decToBcd(uint8_t b)
{
	return ( ((b/10) << 4) + (b%10) );
}


uint8_t RTCx::readData(uint8_t reg) const
{
	Wire.beginTransmission(address);
	Wire.write(reg);
	Wire.endTransmission();
	Wire.requestFrom(address, uint8_t(1));
	uint8_t value = Wire.read();
	Wire.endTransmission();
	return value;
}


void RTCx::writeData(uint8_t reg, uint8_t value) const
{
	Wire.beginTransmission(address);
	Wire.write(reg);
	Wire.write(value);
	Wire.endTransmission();
}


uint8_t RTCx::getRegister(timeFunc_t func, uint8_t &sz) const
{
	const uint8_t regTable[3][5] = {
		{0, 0xff, 0xff, 0xff, 0xff}, // DS1307
		{0, 0x0a, 0x11, 0x18, 0x1C}, // MCP7941x
		{1, 0x08, 0x0d, 0xff, 0xff}, // PCF85263

	};
	const uint8_t szTable[3][5] = {
		{7, 0, 0, 0, 0}, // DS1307
		{7, 6, 6, 4, 4}, // MCP7941x
		{7, 0, 0, 0, 0}, // PCF85263. Alarms not supported.
	};
	sz = szTable[device][func];
	return regTable[device][func];
	//return pgm_read_byte((regTable[device][func]));
}


bool RTCx::readTimeSaver(struct tm *tm, uint8_t reg, uint8_t sz) const
{
	// Reset the register pointer
	Wire.beginTransmission(address);
	Wire.write(reg);
	Wire.endTransmission();

	Wire.requestFrom(address, sz);
	tm->tm_sec = 0;
	tm->tm_min = bcdToDec(Wire.read() & 0x7f);
	tm->tm_hour = bcdToDec(Wire.read() & 0x3f);
	tm->tm_wday = 0;
	tm->tm_mday = bcdToDec(Wire.read() & 0x3f);
	uint8_t wdayMonth = Wire.read();
	tm->tm_mon = bcdToDec(wdayMonth & 0x1f) - 1; // Clock uses [1..12]
	tm->tm_wday = (wdayMonth >> 5) - 1; // Clock uses [1..7]
	tm->tm_year = (RTCX_EPOCH - 1900); // not stored
	tm->tm_yday = -1;
	Wire.endTransmission();
	return true;
}

int RTCx::isotime(const struct tm *tm, char *buffer, size_t len)
{
	return SNPRINTF(buffer, len, PSTR("%04d-%02d-%02dT%02d:%02d:%02d"),
					  tm->tm_year + 1900,
					  tm->tm_mon + 1,
					  tm->tm_mday,
					  tm->tm_hour,
					  tm->tm_min,
					  tm->tm_sec);
}


Stream& RTCx::printIsotime(Stream &s, const time_t &t)
{
	char buffer[20];
	isotime(t, buffer, sizeof(buffer));
	s.print(buffer);
	return s;
}

Stream& RTCx::printIsotime(Stream &s, const struct tm *tm)
{
	char buffer[20];
	isotime(tm, buffer, sizeof(buffer));
	s.print(buffer);
	return s;
}

RTCX_CONST_STRING_T* RTCx::getDeviceName(device_t device) {
	if (device > sizeof(deviceNames) / sizeof(deviceNames[0]))
		return (RTCX_CONST_STRING_T*)(F("unknown RTC"));
	else {
#ifdef ARDUINO_ARCH_AVR
		return (RTCX_CONST_STRING_T*)(pgm_read_ptr(&(deviceNames[device])));
#else
		return deviceNames[device];
#endif
	}
}
