#ifndef RELAY_CONTROLLER_WIFI_CONFIG
	#define RELAY_CONTROLLER_WIFI_CONFIG	

	/********* START CONFIG *******************************/

	// System ID
	//const String roomID = "kroom";

	// Network credentials
	//const char* ssid = "";
	//const char* password = "";

	// MQTT Broker:
	//const char *mqtt_server = "";
	//const char *mqtt_username = "";
	//const char *mqtt_password = "";

	// Esp-01
	#define RXD2 3
	#define TXD2 1
	//#define WIFI_LED_PIN 2
	//#define MQTT_LED_PIN 0	
	
	//#define RXD2 3
	//#define TXD2 1
	
	// Esp32 dev kit
	//#define RXD2 16
	//#define TXD2 17
	//#define WIFI_LED_PIN 18
	//#define MQTT_LED_PIN 19
	
	#define BAUD_RATE 4800

	// Serial debugging
	#ifndef START_SERIAL_DEBUG
		#define START_SERIAL_DEBUG 0
	#endif
	
	/************ END CONFIG ******************************/

#endif