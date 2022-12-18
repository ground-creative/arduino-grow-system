#ifndef MAIN_CONTROLLER_CUSTOM_CONFIG

	#define I2C_ADDRESS 0x3C
	#define RST_PIN -1
	#define TDS_PIN 35
	#define ESPADC 4096.0   //the esp Analog Digital Convertion value
	#define ESPVOLTAGE 3300 //the esp voltage supply value
	#define PH_PIN 32    //the esp gpio data pin number
	#define DS18B20_PIN  23
	#define WIFI_LED_PIN 18
	#define MQTT_LED_PIN 19
	
	const int tdsFlashAddress = 8;
	const int oledFlashAddress = 20;
	const int updateIntervalFlashAddress = 30;
	const int nightModeFlashAddress = 50;
	const int waterTempSensorMaxRetries = 10;
	unsigned long checkConnectionInterval = 5000;
	
#endif

#ifndef USE_TEMP
	#define USE_TEMP 1
#endif

#ifndef USE_PH
	#define USE_PH 1
#endif

#ifndef USE_TDS
	#define USE_TDS 1
#endif
