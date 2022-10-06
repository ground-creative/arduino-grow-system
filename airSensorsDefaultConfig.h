#ifndef AIR_SENSORS_CONFIG
	#define AIR_SENSORS_CONFIG	

	// ds1820 sensor pin
	#define DS1820_PIN  2

	// dht22 sensor pin
	#define DHT_PIN 3
	#define BAUD_RATE 4800

	#define DHT_TYPE DHT22     // DHT 22 (AM2302)

	#define RXD2  4
	#define TXD2  5 

	// Display I2C address
	#define I2C_ADDRESS 0x3C

	// Define proper RST_PIN if required.
	#define RST_PIN -1

	#define MQ135_PIN     A0                 // analog feed from MQ135 co2 sensor
	#define co2Zero     	0                        // calibrated CO2 0 level
	
	const int oledFlashAddress = 0;
	//const int roomIDFlashAddress = 10;
	//const int componentIDFlashAddress = 30;
	const int mq135ROFlashAddress = 50;
	const int outTempSensorMaxRetries = 10;
	unsigned long updateInterval = 10000; 
	unsigned long co2BaseValue = 400;	
	
	#define BOARD "Arduino UNO"
	#define MQ_TYPE "MQ-135" //MQ135
	#define VOLTAGE_RESOLUTION 5
	#define ADC_BIT_RESOLUTION 10
	#define RATIO_MQ135_CLEAN_AIR 3.6//RS / R0 = 3.6 ppm  
	
	//#define CALIBRATE_MQ135 1

#endif

#ifndef USE_DS1820
	#define USE_DS1820 1
#endif	

#ifndef USE_DHT
	#define USE_DHT 1
#endif	

#ifndef USE_MQ135
	#define USE_MQ135 1
#endif