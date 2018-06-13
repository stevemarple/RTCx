/*
 * Print the date, day of week and day of year for the first day of each month for 68 years starting from the epoch.
 */

#include <string.h>
#include <RTCx.h>


const char *days[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


void printDayOfWeek(Stream &s, uint8_t wday)
{
    if (wday >= 0 && wday < 7) {
        s.print(days[wday]);
    }
    else
        s.print("ERROR");
}

void setup(void)
{
#if F_CPU >= 12000000UL
    Serial.begin(115200);
#else
	Serial.begin(9600);
#endif
    delay(50);
    Serial.println();
    Serial.println(F("# START"));
    Serial.print(F("# EPOCH: "));
    Serial.println((int)RTCX_EPOCH);

    for (int year = RTCX_EPOCH; year < RTCX_EPOCH + 68; ++year) {
        for (uint8_t month = 1; month <= 12; ++month) {
            struct RTCx::tm tm;
            memset(&tm, 0, sizeof(tm));

            tm.tm_year = year - 1900; // Years since 1900
            tm.tm_mon = month - 1;    // Months are 0..11
            tm.tm_mday = 1;

            RTCx::time_t t = RTCx::mktime(tm);

            RTCx::printIsotime(Serial, tm);
            Serial.print("  ");
            Serial.print(t);
            Serial.print("  ");
            Serial.print(tm.tm_yday + 1);  // Range 1..366 (strftime %j expansion)
            Serial.print("  ");
            Serial.print(tm.tm_wday);      // Range 0..6 (strftime %w expansion)
            Serial.print("  ");
            printDayOfWeek(Serial, tm.tm_wday);
            Serial.println();
            delay(10);
        }

    }

    Serial.println(F("# END"));
}


void loop(void)
{


}
