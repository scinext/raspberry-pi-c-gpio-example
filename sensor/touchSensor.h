
#ifndef TOUCH_SENSOR_H
#define TOUCH_SENSORH


typedef void (*OnTouchCallback)();
//touchsensor
	void TouchSensorTest();
	void onTouchCallbackTest();
	
	
void TouchSensorInit();
void TouchSensorStart(OnTouchCallback callback);
void TouchSensorEnd();
int TouchSensorInterrupt(int pin, int value);
	
#endif
