#ifndef WIFI_CONFIG
	#define WIFI_CONFIG	
	
	// Esp-01
	#define RXD2 3
	#define TXD2 1
	//#define WIFI_LED_PIN 2
	//#define MQTT_LED_PIN 0	
	
	// Esp32 dev kit
	//#define RXD2 16
	//#define TXD2 17
	//#define WIFI_LED_PIN 18
	//#define MQTT_LED_PIN 19
	
	#define BAUD_RATE 4800
	
	const int roomIDFlashAddress = 0;
	const int componentIDFlashAddress = 20;

#endif