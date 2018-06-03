# RTCx

Arduino library to support DS1307 and similar I2C real-time
clocks. The library can autoprobe to find the actual hardware based on
I2C address.

The library also provides equivalents to the Unix `mktime()` and `gmtime_r()` (a
reentrant version of `gmtime()`). By default the Unix epoch of 1970-01-01 is used. 
An alternative epoch can be defined at compile-time; it must differ from 1970 by an
exact multiple of four years.

## Supported devices

*  DS1307 (also DS1337, DS1338)
*  MCP7941x (MCP79410, MCP79411, MCP79412)
*  PCF85263(A)

## License
Released under the GNU Lesser General Public License, version 2.1. See
LICENSE.txt for details.

## Examples

### RTCx_example

Demonstrate the use of `autoprobe()`, `getDevice()`, `getAddress()` to
identify the first RTC found. Configure the device as appropriate
using `enableBatteryBackup()` and `startClock()`. Enable square-wave
output with `setSQW()`.

## Credits

Thanks to Ian Chilton for
[mcp7941x_arduino](https://github.com/ichilton/mcp7941x_arduino) which
was the starting point for the MCP7941x implementation.
