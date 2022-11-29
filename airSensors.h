/**
  Grow system air sensors component
  Author: Ground Creative 
*/

#define _VERSION_ "1.0.0"
#include "airSensorsDefaultConfig.h"
#include <NetTools.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <MQUnifiedsensor.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

AsyncWebServer server(80);

DHT dht(DHT_PIN, DHT_TYPE);
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
MQUnifiedsensor MQ135(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ135_PIN, MQ_TYPE);
SSD1306AsciiWire oled;

// Component ID
String componentID = "air-sensors";

TaskHandle_t netClient;
NetTools::WIFI network(ssid, password);

String mqttClientID = roomID + "-" + componentID;

float outTemp = 0.0, inTemp = 0.0, inHum = 0.0, inCo2 = 0, RO = 0;
unsigned long previousMillis = 0, netPreviousMillis = 0, updateInterval = 10000; 
unsigned int outTempSensorCountRetries = 0;
bool wifiConnected = false, oledOn = true;
String wifiIP = "";

void calibrateMQ135Sensor()
{
	/*****************************  MQ Calibration ********************************************/ 
	// Explanation: 
	// In this routine the sensor will measure the resistance of the sensor supposedly before being pre-heated
	// and on clean air (Calibration conditions), setting up R0 value.
	// We recomend executing this routine only on setup in laboratory conditions.
	// This routine does not need to be executed on each restart, you can load your R0 value from eeprom.
	// Acknowledgements: https://jayconsystems.com/blog/understanding-a-gas-sensor
	Serial.print("Calibrating please wait.");
	float calcR0 = 0;
	for(int i = 1; i<=10; i ++)
	{
		MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
		calcR0 += MQ135.calibrate(RATIO_MQ135_CLEAN_AIR);
		Serial.print(".");
	}
	EEPROM.put(mq135ROFlashAddress, calcR0/10);
	Serial.print("Saved R0 value: ");
	Serial.print(calcR0/10);
	Serial.println("  done!.");
	oled.clear();
	oled.setRow(1);
	oled.setCursor(0, 0);
	oled.set1X();
	oled.print("cal MQ135 R0 ");
	oled.setRow(2);
	oled.setCursor(10, 10);
	oled.set2X();
	oled.print(calcR0/10);
	delay(1000);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{  
	Serial.println("");
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	String content = "";
	for (int i = 0; i < length; i++) 
	{
		content += (char)payload[i];
		Serial.print((char)payload[i]);
	}
	Serial.println("");
	if (String(topic) == roomID + "/" + componentID + "-restart")
	{
		ESP.restart();
	}
	else if (String(topic) == roomID + "/air-sensors-display-backlight")
	{
		oledOn = content.toInt();
		if (oledOn)
		{
			Serial.println("Turning on lcd");
		}
		else
		{
			Serial.println("Turning off lcd");
			oled.clear();
		}
		EEPROM.write(oledFlashAddress, oledOn);
		EEPROM.commit();
	}
	else if (String(topic) == roomID + "/air-sensors-display-update-interval")
	{
		updateInterval = content.toInt();
		Serial.print("Setting sensors update interval to ");
		Serial.print(updateInterval);
		EEPROM.put(updateIntervalFlashAddress, updateInterval);
		EEPROM.commit();
	}
	else if ( String(topic) == roomID + "/" + componentID + "/calibrate-mq135")
	{
		calibrateMQ135Sensor();
	}
}

NetTools::MQTT mqtt(mqtt_server, mqtt_callback);

void mqttSubscribe(const String& roomID)
{
	Serial.println("Subscribing to mqtt messages");
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-backlight").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-update-interval").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-restart").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/calibrate-mq135").c_str()));
}

void netClientHandler( void * pvParameters )
{
	Serial.print("netClientHandler running on core ");
	Serial.println(xPortGetCoreID());
	for (;;)
	{
		unsigned long currentMillis = millis();
		if(!wifiConnected || currentMillis - netPreviousMillis >= checkConnectionInterval) 
		{
			//Serial.println("Checking connection");
			netPreviousMillis = currentMillis;
			if (!wifiConnected)
			{
				network.connect();
				digitalWrite(WIFI_LED_PIN, LOW);
				oled.clear();
				if (oledOn)
				{
					oled.setRow(1);
					oled.setCursor(0, 0);
					oled.set1X();
					oled.print("Connected to WiFi");
				}
				wifiIP = network.localAddress().toString();
			}
			else if (network.status() != WL_CONNECTED)
			{
				digitalWrite(WIFI_LED_PIN, HIGH);
				digitalWrite(MQTT_LED_PIN, HIGH);
				wifiIP = "";
				network.check();
				while (network.status( ) != WL_CONNECTED)
				{
					Serial.print('.');
					delay(1000);
				}
				digitalWrite(WIFI_LED_PIN, LOW);
				oled.clear();
				if (oledOn)
				{
					oled.setRow(1);
					oled.setCursor(0, 0);
					oled.set1X();
					oled.print("Connected to WiFi");
				}
				wifiIP = network.localAddress().toString();
			}
			if (!mqtt.isConnected())
			{
				digitalWrite(MQTT_LED_PIN, HIGH);
				if (mqtt.connect(mqttClientID, mqtt_username, mqtt_password))
				{
					mqttSubscribe(roomID);
					digitalWrite(MQTT_LED_PIN, LOW);
					delay(1000);
				}
			}
			wifiConnected = true;
		}
		mqtt.loop();
		delay(100);
	}
}

void updateCo2Values()
{
	if (!USE_MQ135){ return; }
	MQ135.update(); 				// Update data, the arduino will read the voltage from the analog pin
	MQ135.setA(110.47); 			// Configure the equation to calculate CO2 concentration value
	MQ135.setB(-2.862); 			// Configure the equation to calculate CO2 concentration value
	float CO2 = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
	inCo2 = CO2 + co2BaseValue;
	Serial.print("MQ135 CO2: ");
	Serial.print(inCo2);
	Serial.println();
}

void updateDisplayValues()
{
	if (!oledOn)
	{
		return;
	}
	oled.clear();
	oled.setRow(1);
	oled.setCursor(27, 0);
	oled.set1X();
	oled.print(wifiIP);
	oled.setRow(2);
	oled.set2X();
	oled.setCursor(30, 10);
	/*if (outTemp > 0.0)
	{
	led.println(String(inTemp,1) + "C" + " " + String(outTemp,1) + "C");
	}
	else
	{
	oled.println(String(inTemp,1) + "C");
	}*/
	oled.println(String(inTemp, 1) + "C");
	oled.setRow(4);
	oled.setCursor(30, 20);
	oled.println(String(inHum, 1) + "%");
	oled.setRow(6);
	oled.setCursor(30, 40);
	oled.println(String(inCo2, 0) + "ppm");
}

void updateOutTemp()
{
	if (!USE_DS18B20)
	{ 
		return; 
	}
	sensors.requestTemperatures(); 
	float outTempMem = sensors.getTempCByIndex(0);
	if (outTempMem > -127.00)
	{
		outTemp = outTempMem;
		outTempSensorCountRetries = 0;
	}
	else if (outTempSensorCountRetries == outTempSensorMaxRetries)
	{
		Serial.println("Cannot read temperature DS18B20 sensor problem, resetting");
		//delay( 1000 );
		ESP.restart();
	}
	else
	{      
		outTempSensorCountRetries++;
		Serial.print("Cannot read temperature DS1820, tried ");
		Serial.print(outTempSensorCountRetries);
		Serial.print(" times");
		Serial.println();
	}
	Serial.print("DS1820 Temp: ");
	Serial.print(outTemp);
	Serial.println();
}

void updateInTempAndHum()
{
	if (!USE_DHT){ return; }
	float inTempMem = dht.readTemperature();
	if ( isnan(inTempMem) ) 
	{
		Serial.println("Failed to read temperature from DHT sensor!");
	}
	else
	{
		inTemp = inTempMem;
		Serial.print("DH22 Temp: ");
		Serial.print(inTemp);
		Serial.println();
	}
	float inHumMem = dht.readHumidity();
	if ( isnan(inHumMem) ) 
	{
		Serial.println("Failed to read humidity from DHT sensor!");
	}
	else
	{
		inHum = inHumMem;
		Serial.print("DH22 Humidity: ");
		Serial.print(inHum);
		Serial.println();
	}
}

void publishValues()
{
	StaticJsonDocument<256> doc;
	doc["o"] = String(outTemp, 1);
	doc["i"] = String(inTemp, 1);
	doc["h"] = String(inHum, 1);
	doc["c"] = String(inCo2, 0);
	char buffer[256];
	serializeJson(doc, buffer);
	mqtt.publish(const_cast<char*>(String(roomID + "/air-sensors").c_str()) , buffer);
}

void setup() 
{
	Serial.begin(9600);
	EEPROM.begin(255);
	delay(1000);
	Serial.println("Component started with config " + roomID +  ":" + componentID);
	pinMode(WIFI_LED_PIN, OUTPUT);
	digitalWrite(WIFI_LED_PIN, HIGH);
	pinMode(MQTT_LED_PIN, OUTPUT);
	digitalWrite(MQTT_LED_PIN, HIGH);
	Wire.begin();
	//Wire.setClock(400000L);
	#if RST_PIN >= 0
	oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
	#else // RST_PIN >= 0
	oled.begin(&Adafruit128x64, I2C_ADDRESS);
	#endif // RST_PIN >= 0
	oled.setFont(Adafruit5x7);
	oled.clear();
	oledOn = EEPROM.read( oledFlashAddress );
	updateInterval = EEPROM.get(updateIntervalFlashAddress, updateInterval);
	if (!updateInterval || updateInterval < 1000 || updateInterval > 400000000)
	{
		updateInterval = 10000;
	}
	oled.setRow(1);
	oled.setCursor(10, 0);
	oled.print(roomID);
	oled.set1X();
	oled.setRow(3);
	oled.setCursor(10, 10);
	oled.set2X();
	oled.print("Starting");
	if (!oledOn)
	{
		delay(1000);
		oled.clear();
	}
	if (USE_DS18B20){ sensors.begin( ); }
	if (USE_DHT){ dht.begin(); }
	if (USE_MQ135)
	{
		MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b
		MQ135.init(); 
		MQ135.setRL(1);	// value is different from 10K
		#ifdef CALIBRATE_MQ135
			calibrateMQ135Sensor();
		#endif
		EEPROM.get(mq135ROFlashAddress, RO);
		if ( isnan(RO) )
		{
			Serial.println("No RO found for co2 mq135 sensor, please calibrate");
			RO = 1.0;
		}
		MQ135.setR0(RO);
	}
	xTaskCreatePinnedToCore
	(
		netClientHandler,     	/* Task function. */
		"netClient",          		/* name of task. */
		10000,                		/* Stack size of task */
		NULL,                 		/* parameter of the task */
		1,                    			/* priority of the task */
		&netClient,          		/* Task handle to keep track of created task */
		0                     			/* pin task to core 0 */   
	);                        
	delay(500); 
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) 
	{
		request->send(200, "text/plain", roomID + ":" + componentID + " " + " v" + String(_VERSION_) );
	} );
	AsyncElegantOTA.begin(&server);    // Start ElegantOTA
	server.begin();
	Serial.println("HTTP server started");
}

void loop() 
{
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis >= updateInterval) 
	{
		previousMillis = currentMillis;
		updateOutTemp();
		updateInTempAndHum();
		updateCo2Values();
		updateDisplayValues(); 
		publishValues();
	}
}