#ifndef RELAY_CONTROLLER_CONFIG_
	#define RELAY_CONTROLLER_CONFIG	

	/********* START CONFIG *******************************/

	// System ID
	//String roomID = "koh-chang";

	// LCD
	int lcdColumns = 20;
	int lcdRows = 4;
	uint8_t lcdAddress = 0x3F;

	// Solenoid Valve
	#define SOLENOID_VALVE_RELAY_PIN  P0
	#define SOLENOID_VALVE_LED_PIN  19
	#define SOLENOID_VALVE_BTN_PIN  23
	int solValveRelayState = HIGH;
	const int solValveFlashAddress = 0;

	// Drain Pump
	#define DRAIN_PUMP_RELAY_PIN  P1
	#define DRAIN_PUMP_LED_PIN  5
	#define DRAIN_PUMP_BTN_PIN  18
	int drainPumpRelayState = HIGH;
	const int drainPumpFlashAddress = 1;

	// Mixing Pump
	#define MIXING_PUMP_RELAY_PIN  P2
	#define MIXING_PUMP_LED_PIN  15
	#define MIXING_PUMP_BTN_PIN  2
	int mixingPumpRelayState = HIGH;
	const int mixingPumpFlashAddress = 2;

	// Extractor
	#define EXTRACTOR_RELAY_PIN  P3
	#define EXTRACTOR_LED_PIN  13
	#define EXTRACTOR_BTN_PIN  4
	int extractorRelayState = HIGH;
	const int extractorFlashAddress = 3;

	// Lights
	#define LIGHTS_RELAY_PIN  P4
	#define LIGHTS_LED_PIN  14
	#define LIGHTS_BTN_PIN  27
	int lightsRelayState = HIGH;
	const int lightsFlashAddress = 4;

	// Feeding Pump
	#define FEEDING_PUMP_RELAY_PIN  P5
	#define FEEDING_PUMP_LED_PIN  26
	#define FEEDING_PUMP_BTN_PIN  25
	int feedingPumpRelayState = HIGH;
	const int feedingPumpFlashAddress = 5;

	// Fan
	#define FAN_RELAY_PIN  P6
	#define FAN_LED_PIN  33
	#define FAN_BTN_PIN  32
	int fanRelayState = HIGH;
	const int fanFlashAddress = 6;

	// Airco
	#define AIRCO_RELAY_PIN  P7
	#define AIRCO_LED_PIN  0
	#define AIRCO_BTN_PIN  12

	int aircoRelayState = HIGH;
	const int aircoFlashAddress = 7;
	
	// Reset button
	#define RESET_PIN  3
	int resetButtonState = HIGH; 
	long lastDebounceTime = 0;
	long debounceDelay = 200;

	// Wifi chip tx and rx
	#define RXD2 16
	#define TXD2 17

	// Serial2 baud rate
	#define BAUD_RATE 4800

	// Update values on display interval in milliseconds
	unsigned long updateInterval;
	const int updateIntervalFlashAddress = 8;
	
	// Display backlight status
	bool backlightOn;
	const int backlightOnFlashAddress = updateIntervalFlashAddress + sizeof(updateIntervalFlashAddress) +1;

	/************ END CONFIG ******************************/

#endif