#ifndef AIR_SENSORS_CUSTOM_CONFIG

  #define I2C_ADDRESS 0x3C
  #define RST_PIN -1
  #define DHT_TYPE DHT22
  #define DS18B20_PIN 16
  #define DHT_PIN 17
  #define MQ135_PIN 32 
  #define WIFI_LED_PIN 19
  #define MQTT_LED_PIN 18
  
  #define BOARD "ESP-32"
  #define MQ_TYPE "MQ-135" 				
  #define VOLTAGE_RESOLUTION 3.3
  #define ADC_BIT_RESOLUTION 12
  #define RATIO_MQ135_CLEAN_AIR 3.6	//RS / R0 = 3.6 ppm  
  
  const int oledFlashAddress = 20;
  const int updateIntervalFlashAddress = 30;
  const int mq135ROFlashAddress = 50;
  const int addressECO2 = 70;  
  const int addressTVOC = 90;
  const int nightModeFlashAddress = 110;
  const int outTempSensorMaxRetries = 10;
  unsigned long checkConnectionInterval = 5000;
  unsigned long co2BaseValue = 400;
  unsigned long baselineSavePeriod = 10800000; // 3h
  
#endif

#ifndef USE_DS18B20
  #define USE_DS18B20 1
#endif

#ifndef USE_DHT
  #define USE_DHT 1
#endif

#ifndef USE_MQ135
  #define USE_MQ135 0
#endif

#ifndef USE_SGP30
  #define USE_SGP30 1
#endif