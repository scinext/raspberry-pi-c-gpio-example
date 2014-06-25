
#ifndef TOUCH_SENSOR_H
#define TOUCH_SENSORH


typedef void (*OnTouchCallback)(unsigned int cap);
//touchsensor
	void TouchSensorTest();
	void onTouchCallbackTest(unsigned int cap);
	
	
void TouchSensorInit();
void TouchSensorStart(OnTouchCallback callback);
void TouchSensorEnd();
int TouchSensorInterrupt(int pin, int value);
	
#endif
