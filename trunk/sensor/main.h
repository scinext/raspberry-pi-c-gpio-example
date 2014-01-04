
#ifndef MAIN_H
#define MAIN_H

#define MODE_TEMP		0x01
#define MODE_PRESS		0x02
#define MODE_HUMIDITY	0x03
#define MODE_LUX		0x04
#define MODE_LUX_OHM	0x05
#define MODE_ALL		0xFFFF

#define DebugModePrintf(str, ...)	if( g_outLevel == 1 ) printf(str, ##__VA_ARGS__);
#define SensorLogPrintf(level, str, ...) DebugModePrintf(str, ##__VA_ARGS__)

#endif
