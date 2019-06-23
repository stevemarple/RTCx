#ifndef RTCX_h
#define RTCX_h

#define RTCX_VERSION "1.1.10"


#if defined(__GNUC__) || defined(__clang__)
#define DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED __declspec(deprecated)
#else
// Deprecated warnings are not available for this compiler
#define DEPRECATED
#endif


#include <stdint.h>

#ifndef RTCX_EPOCH
#define RTCX_EPOCH 1970
#endif

#if (RTCX_EPOCH <= 1900 || RTCX_EPOCH >= 2100)
#error RTCX_EPOCH must be > 1900 and < 2100
#endif

// This restriction comes from assumptions made in mktime() and gmtime_r() to calculate day of year.
#if ((RTCX_EPOCH - 1970) % 4 != 0)
#error RTCX_EPOCH must be 1970 or differ from 1970 by a multiple of 4 years
#endif

class RTCx;
extern RTCx rtc;

#define RTCX_NUM_SUPPORTED_DEVICES 3

#ifdef ARDUINO_ARCH_AVR
#define RTCX_CONST_STRING_T const __FlashStringHelper
#else
#define RTCX_CONST_STRING_T const char
#endif

class RTCx {
public:
	enum device_t {
		DS1307 = 0, // Include compatible devices (eg DS1337, DS1338)
		MCP7941x = 1,
		PCF85263 = 2,
		// If adding more devices remember to update
		// RTCX_NUM_SUPPORTED_DEVICES.
	};



	enum timeFunc_t {
		TIME = 0,
		ALARM0 = 1,
		ALARM1 = 2,
		TIME_POWER_FAILED = 3,
		TIME_POWER_RESTORED = 4,
	};

	enum freq_t {
		freq1Hz = 0,
		freq4096Hz = 1,
		freq8192Hz = 2,
		freq32768Hz = 3,
		freqCalibration = 4, // device-specific calibration for MCP7941x

		// Additional values for PCF85263
		freq1024Hz = 5,
		freq2048Hz = 6,
		freq16384Hz = 7,
		freqOutputLow = 8,
	};

	static const uint8_t epochDow; // Day of the week for the epoch date

	static const uint8_t DS1307Address;
	static const uint8_t MCP7941xAddress;
	static const uint8_t MCP7941xEepromAddress;
	static const uint8_t PCF85263Address;

	static const char DS1307Str[] PROGMEM;
	static const char MCP7941xStr[] PROGMEM;
	static const char PCF85263Str[] PROGMEM;
	static PGM_P const deviceNames[] PROGMEM;

	// Devices which can be autoprobed and their recommended order
	static const device_t autoprobeDeviceList[3];
	static const uint8_t autoprobeDeviceAddresses[3];

	void init(void) const;
	bool resetClock(void) const;

	struct tm {
		int tm_sec; // Seconds [0..59]
		int tm_min; // Minutes [0..59].
		int tm_hour; // Hour [0..23].
		int tm_mday; // Day of month [1..31].
		int tm_mon; // Month of year [0..11].
		int tm_year; // Years since 1900.
		int tm_wday; // Day of week [0..6] (Sunday=0).
		int tm_yday; // Day of year [0..365]. (-1=unset).
		int tm_isdst; // Daylight Savings flag (ignored).
	};

	typedef int32_t time_t;

	static bool isLeapYear(uint16_t year);
	static uint8_t daysInMonth(uint16_t year, uint8_t month);
	static uint16_t dayOfYear(uint16_t year, uint8_t month, uint8_t day);
	static time_t mktime(struct tm *tm);
	inline static time_t mktime(struct tm &tm);
	static struct tm *gmtime_r(const time_t *timep, struct tm *result);

	inline static int isotime(const time_t &t, char *buffer, size_t len);
	inline static int isotime(const struct tm &tm, char *buffer, size_t len);
	static int isotime(const struct tm *tm, char *buffer, size_t len);
	static Stream& printIsotime(Stream &s, const time_t &t);
	inline static Stream& printIsotime(Stream &s, const struct tm &tm);
	static Stream& printIsotime(Stream &s, const struct tm *tm);
	static RTCX_CONST_STRING_T* getDeviceName(device_t device);

	
	RTCx(void);
	RTCx(uint8_t a);
	RTCx(uint8_t a, device_t d);

	inline bool autoprobe(void) {
		return autoprobe(&autoprobeDeviceList[0], autoprobeDeviceAddresses, sizeof(autoprobeDeviceList));
	};
	bool autoprobe(const device_t *deviceList, const uint8_t *addressList, uint8_t len);

	DEPRECATED bool autoprobe(const uint8_t *addressList, uint8_t len);

	void stopClock(void) const;
	inline void startClock(void) const {
		startClock((int16_t)-1);
	}

	bool readClock(struct tm *tm, timeFunc_t func = TIME) const;
	inline bool readClock(struct tm &tm, timeFunc_t func = TIME) const;
	bool readClock(char *buffer, size_t len, timeFunc_t func = TIME) const;
	bool setClock(const struct tm *tm, timeFunc_t func = TIME) const;
	inline bool setClock(const struct tm &tm, timeFunc_t func = TIME) const;
	bool setClock(const char* ISOTimestamp, timeFunc_t func = TIME) const;
	bool adjustClock(time_t offset) const;

	inline uint8_t getAddress(void) const;
	inline void setAddress(uint8_t a);
	inline device_t getDevice(void) const;
	inline void setDevice(device_t d);
	inline RTCX_CONST_STRING_T* getDeviceName(void) const {
		return getDeviceName(device);
	}

	bool setSQW(freq_t f) const;

	// Functions for MCP7941x
	void enableBatteryBackup(bool enable = true) const;
	bool getPowerFailFlag(void) const;
	void clearPowerFailFlag(void) const;
	int8_t getCalibration(void) const;
	bool setCalibration(int8_t cal) const;

private:
	uint8_t address;
	device_t device;

	static uint8_t bcdToDec(uint8_t b);
	static uint8_t decToBcd(uint8_t b);

	void startClock(int16_t bcdSec) const;
	uint8_t readData(uint8_t reg) const;
	void writeData(uint8_t reg, uint8_t value) const;
	uint8_t getRegister(timeFunc_t func, uint8_t &sz) const;
	bool readTimeSaver(struct tm *tm, uint8_t reg, uint8_t sz) const;
};

RTCx::time_t RTCx::mktime(struct tm &tm) {
	return mktime(&tm);
}

inline uint8_t RTCx::getAddress(void) const
{
	return address;
}

inline void RTCx::setAddress(uint8_t a)
{
	address = a;
}

inline RTCx::device_t RTCx::getDevice(void) const
{
	return device;
}

inline void RTCx::setDevice(device_t d)
{
	device = d;
}

inline bool RTCx::readClock(struct tm &tm, timeFunc_t func) const
{
	return readClock(&tm, func);
}

inline bool RTCx::setClock(const struct tm &tm, timeFunc_t func) const
{
	return setClock(&tm, func);
}

inline int RTCx::isotime(const time_t &t, char *buffer, size_t len)
{
	struct tm tm;
	gmtime_r(&t, &tm);
	return isotime(&tm, buffer, len);
}

inline int RTCx::isotime(const struct tm &tm, char *buffer, size_t len)
{
	return isotime(&tm, buffer, len);
}

inline Stream& RTCx::printIsotime(Stream &s, const struct tm &tm)
{
	return printIsotime(s, &tm);
}

#undef DEPRECATED
#endif
