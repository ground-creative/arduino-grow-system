#ifndef MAIN_CONTROLLER_CUSTOM_CONFIG	

	/********* START PINS CONFIG *******************************/

	// Solenoid Valve
	#define SOLENOID_VALVE_RELAY_PIN  19
	#define SOLENOID_VALVE_BTN_PIN  23
	
	// Drain Pump
	#define DRAIN_PUMP_RELAY_PIN  5
	#define DRAIN_PUMP_BTN_PIN  18
	
	// Mixing Pump	
	#define MIXING_PUMP_RELAY_PIN  15 
	#define MIXING_PUMP_BTN_PIN  2
	
	// Extractor
	#define EXTRACTOR_RELAY_PIN   13
	#define EXTRACTOR_BTN_PIN  4
	
	// Lights
	#define LIGHTS_RELAY_PIN  14
	#define LIGHTS_BTN_PIN  27

	// Feeding Pump
	#define FEEDING_PUMP_RELAY_PIN 26
	#define FEEDING_PUMP_BTN_PIN  25

	// Fan
	#define FAN_RELAY_PIN  33
	#define FAN_BTN_PIN  32

	// Airco
	#define AIRCO_RELAY_PIN  0
	#define AIRCO_BTN_PIN  12
	
	// Reset button
	#define LCD_PIN  3
	
	#define WIFI_LED_PIN 16
	#define MQTT_LED_PIN 17

	// Check WiFi connection interval
	unsigned long checkConnectionInterval = 5000;

	/************ END PINS CONFIG ******************************/

#endif

#ifndef LCD_CUSTOM_CONFIG

	/********* START LCD CONFIG *******************************/

	int lcdColumns = 20;
	int lcdRows = 4;
	uint8_t lcdAddress = 0x27;
	
	/************ END LCD CONFIG ******************************/
	
#endif