/**
  Grow system air sensors component
  Author: Ground Creative 
*/

#define _VERSION_ "1.2.2"
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
#include <WebSerial.h>
#include <Wire.h>
#include "Adafruit_SGP30.h"

AsyncWebServer server(80);

DHT dht(DHT_PIN, DHT_TYPE);
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
MQUnifiedsensor MQ135(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ135_PIN, MQ_TYPE);
Adafruit_SGP30 sgp;
SSD1306AsciiWire oled;

// Component ID
String componentID = "air-sensors";

TaskHandle_t netClient;
NetTools::WIFI network(ssid, password);

String mqttClientID = roomID + "-" + componentID;

float outTemp = 0.0, inTemp = 0.0, inHum = 0.0, inCo2 = 0, RO = 0;
unsigned long previousMillis = 0, netPreviousMillis = 0, updateInterval = 10000; 
unsigned int outTempSensorCountRetries = 0,restoredECO2Baseline = 0,restoredTVOCBaseline = 0, counter = 0;
bool wifiConnected = false, mqttConnected = false, oledOn = true, nightMode = false;
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
	WebSerial.print("Calibrating please wait.");
	float calcR0 = 0;
	for (int i = 1; i<=10; i ++)
	{
		MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
		calcR0 += MQ135.calibrate(RATIO_MQ135_CLEAN_AIR);
		Serial.print(".");
	}
	EEPROM.put(mq135ROFlashAddress, calcR0/10);
	EEPROM.commit();
	Serial.print("Saved R0 value: ");
	Serial.print(calcR0/10);
	Serial.println("  done!.");
	WebSerial.print("Saved R0 value: ");
	WebSerial.print(calcR0/10);
	WebSerial.println("  done!.");
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

uint32_t getAbsoluteHumidity(float temperature, float humidity) 
{
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{  
	Serial.println(""); Serial.print("Message arrived ["); Serial.print(topic); Serial.print("] ");
	WebSerial.print("Message arrived ["); WebSerial.print(topic); WebSerial.print("] ");
	String content = "";
	for (int i = 0; i < length; i++) 
	{
		content += (char)payload[i];
		Serial.print((char)payload[i]);
	}
	//Serial.println("");
	WebSerial.print(content);
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
			WebSerial.println("Turning on lcd");
		}
		else
		{
			Serial.println("Turning off lcd");
			WebSerial.println("Turning off lcd");
			oled.clear();
		}
		EEPROM.write(oledFlashAddress, oledOn);
		EEPROM.commit();
	}
	else if (String(topic) == roomID + "/air-sensors-night-mode")
	{
		nightMode = content.toInt();
		if (nightMode)
		{
			Serial.println("Turning on night mode");
			WebSerial.println("Turning on night mode");
			digitalWrite(WIFI_LED_PIN, HIGH);
			digitalWrite(MQTT_LED_PIN, HIGH);
		}
		else
		{
			Serial.println("Turning off night mode");
			WebSerial.println("Turning off night mode");
			if (wifiConnected)
			{				
				digitalWrite(WIFI_LED_PIN, LOW);
			}
			if (mqttConnected)
			{	
				digitalWrite(MQTT_LED_PIN, LOW);
			}
		}
		EEPROM.write(nightModeFlashAddress, nightMode);
		EEPROM.commit();
	}
	else if (String(topic) == roomID + "/air-sensors-display-update-interval")
	{
		updateInterval = content.toInt();
		Serial.print("Setting sensors update interval to "); Serial.print(updateInterval);
		WebSerial.print("Setting sensors update interval to "); WebSerial.print(String(updateInterval));
		EEPROM.put(updateIntervalFlashAddress, updateInterval);
		EEPROM.commit();
	}
	else if (String(topic) == roomID + "/" + componentID + "/calibrate-mq135")
	{
		calibrateMQ135Sensor();
	}
	else if(String(topic) == roomID + "/" + componentID + "/calibrate-sgp30")
	{
		Serial.print("Calibrating sgp30");
		WebSerial.print("Calibrating Sgp30");
		EEPROM.put(addressECO2, 0); 
		EEPROM.put(addressTVOC, 0);
		EEPROM.commit();
		delay(1000);
		ESP.restart();
	}
}

NetTools::MQTT mqtt(mqtt_server, mqtt_callback);

void mqttSubscribe(const String& roomID)
{
	Serial.println("Subscribing to mqtt messages");
	WebSerial.println("Subscribing to mqtt messages");
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-night-mode").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-backlight").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-update-interval").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-restart").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/calibrate-mq135").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/calibrate-sgp30").c_str()));
}

void recvMsg(uint8_t *data, size_t len)
{
	WebSerial.println(""); WebSerial.println("Received WebSerial command...");
	Serial.println(""); Serial.println("Received WebSerial command...");
	String d = "";
	String v = "";
	for(int i=0; i < len; i++)
	{
		d += char(data[i]);
	}
	d.toUpperCase();
	WebSerial.println(d);
	if ( d.indexOf(':') > -1 )
	{
		v = d.substring((d.indexOf(':')+1),d.length());
		d = d.substring(0,d.indexOf(':'));
	}
	if(d == "RESTART")
	{
		Serial.print("Restarting");
		WebSerial.print("Restarting");
		delay(3000);
		ESP.restart();
	}
	else if(d == "CALIBRATESGP30")
	{
		Serial.println("Calibrating sgp30");
		WebSerial.print("Calibrating sgp30");
		EEPROM.put(addressECO2, 0); 
		EEPROM.put(addressTVOC, 0);
		EEPROM.commit();
		delay(3000);
		ESP.restart();
	}
	else if(d == "CALIBRATEMQ135")
	{
		calibrateMQ135Sensor();
	}
	else if(d == "UPDATEINTERVAL")
	{
		updateInterval = v.toInt();
		Serial.print("Setting sensors update interval to ");	Serial.print(updateInterval);
		WebSerial.print("Setting sensors update interval to "); WebSerial.print(String(updateInterval));
		EEPROM.put(updateIntervalFlashAddress, updateInterval);
		EEPROM.commit();
	}
	else if(d == "NIGHTMODE")
	{
		nightMode = v.toInt();
		if (nightMode)
		{
			Serial.println("Turning on night mode");
			WebSerial.println("Turning on night mode");
			digitalWrite(WIFI_LED_PIN, HIGH);
			digitalWrite(MQTT_LED_PIN, HIGH);
			mqtt.publish(const_cast<char*>(String("m/" + roomID + "/air-sensors-night-mode").c_str()) , "1");
		}
		else
		{
			Serial.println("Turning off night mode");
			WebSerial.println("Turning off night mode");
			if (wifiConnected)
			{				
				digitalWrite(WIFI_LED_PIN, LOW);
			}
			if (mqttConnected)
			{	
				digitalWrite(MQTT_LED_PIN, LOW);
			}
			mqtt.publish(const_cast<char*>(String("m/" + roomID + "/air-sensors-night-mode").c_str()) , "0");
		}
		EEPROM.write(nightModeFlashAddress, nightMode);
		EEPROM.commit();
	}
	else if(d == "OLEDON")
	{
		oledOn = v.toInt();
		if (oledOn)
		{
			Serial.println("Turning on lcd");
			WebSerial.println("Turning on lcd");
			mqtt.publish(const_cast<char*>(String("m/" + roomID + "/air-sensors-oled-on").c_str()) , "1");
		}
		else
		{
			Serial.println("Turning off lcd");
			WebSerial.println("Turning off lcd");
			oled.clear();
			mqtt.publish(const_cast<char*>(String("m/" + roomID + "/air-sensors-oled-on").c_str()) , "0");
		}
		EEPROM.write(oledFlashAddress, oledOn);
		EEPROM.commit();
	}
}

void netClientHandler( void * pvParameters )
{
	Serial.print("netClientHandler running on core "); Serial.println(xPortGetCoreID());
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
				if (!nightMode)
				{
					digitalWrite(WIFI_LED_PIN, LOW);
				}
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
				if (!nightMode)
				{
					digitalWrite(WIFI_LED_PIN, LOW);
				}
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
				mqttConnected = false;
				if (mqtt.connect(mqttClientID, mqtt_username, mqtt_password))
				{
					mqttConnected = true;
					mqttSubscribe(roomID);
					if (!nightMode)
					{
						digitalWrite(MQTT_LED_PIN, LOW);
					}
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
	if (USE_MQ135)
	{
		MQ135.update(); 				// Update data, the arduino will read the voltage from the analog pin
		MQ135.setA(110.47); 			// Configure the equation to calculate CO2 concentration value
		MQ135.setB(-2.862); 			// Configure the equation to calculate CO2 concentration value
		float CO2 = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
		inCo2 = CO2 + co2BaseValue;
		Serial.print("MQ135 CO2: "); Serial.print(inCo2); Serial.println();
		WebSerial.print("MQ135 CO2: "); WebSerial.print(inCo2); WebSerial.println();
	}
	if (USE_SGP30)
	{
		uint16_t eCO2Baseline;
		uint16_t TVOCBaseline;
		static unsigned long lastBaselineSaving = millis();
		sgp.setHumidity(getAbsoluteHumidity(inTemp, inHum));
		if (! sgp.IAQmeasure())
		{
			Serial.println("SGP30 Measurement failed");
			WebSerial.println("SGP30 Measurement failed");
			inCo2 = 0;
			return;
		}
		inCo2 = sgp.eCO2;
		Serial.print("TVOC "); Serial.print(sgp.TVOC); Serial.print(" ppb\t");
		Serial.print("eCO2 "); Serial.print(sgp.eCO2); Serial.println(" ppm");
		WebSerial.print("TVOC "); WebSerial.print(sgp.TVOC); WebSerial.print(" ppb\t");
		WebSerial.print("eCO2 "); WebSerial.print(sgp.eCO2); WebSerial.println(" ppm");
		if (! sgp.IAQmeasureRaw()) 
		{
			Serial.println("SGP30 Raw Measurement failed");
			WebSerial.println("SGP30 Raw Measurement failed");
			return;
		}
		Serial.print("Raw H2 "); Serial.print(sgp.rawH2); Serial.print(" \t");
		Serial.print("Raw Ethanol "); Serial.print(sgp.rawEthanol); Serial.println("");
		WebSerial.print("Raw H2 "); WebSerial.print(sgp.rawH2); WebSerial.print(" \t");
		WebSerial.print("Raw Ethanol "); WebSerial.print(sgp.rawEthanol); WebSerial.println("");
		//delay(1000);
		counter++;
		if (counter == 30) 
		{
			counter = 0;
			if (! sgp.getIAQBaseline(&eCO2Baseline, &TVOCBaseline)) 
			{
				Serial.println("Failed to get sgp30 baseline readings");
				WebSerial.println("Failed to get sgp30 baseline readings");
				return;
			}
			Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2Baseline, HEX);
			Serial.print(" & TVOC: 0x"); Serial.println(TVOCBaseline, HEX);
			WebSerial.print("****Baseline values: eCO2: 0x"); WebSerial.print(eCO2Baseline);
			WebSerial.print(" & TVOC: 0x"); WebSerial.println(TVOCBaseline);
		}
		if ((millis()-lastBaselineSaving)>baselineSavePeriod)
		{
			Serial.println("Saving sgp30 baseline");
			Serial.print("eCO2 baseline: "); Serial.println(eCO2Baseline,HEX);
			Serial.print("TVOC baseline: "); Serial.println(TVOCBaseline, HEX);
			WebSerial.println("Saving sgp30 baseline");
			WebSerial.print("eCO2 baseline: "); WebSerial.println(eCO2Baseline);
			WebSerial.print("TVOC baseline: "); WebSerial.println(TVOCBaseline);
			sgp.getIAQBaseline(&eCO2Baseline, &TVOCBaseline);
			EEPROM.put(addressECO2, eCO2Baseline); 
			EEPROM.put(addressTVOC, TVOCBaseline);
			lastBaselineSaving = millis();
			EEPROM.commit();
		}
	}
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
		WebSerial.println("Cannot read temperature DS18B20 sensor problem, resetting");
		//delay( 1000 );
		ESP.restart();
	}
	else
	{      
		outTempSensorCountRetries++;
		Serial.print("Cannot read temperature DS18B20, tried ");
		Serial.print(outTempSensorCountRetries);
		Serial.print(" times");
		Serial.println();
		WebSerial.print("Cannot read temperature DS18B20, tried ");
		WebSerial.print(outTempSensorCountRetries);
		WebSerial.print(" times");
		WebSerial.println();
	}
	Serial.print("DS18B20 Temp: "); Serial.print(outTemp); Serial.println();
	WebSerial.print("DS18B20 Temp: "); WebSerial.print(outTemp); WebSerial.println();
}

void updateInTempAndHum()
{
	if (!USE_DHT){ return; }
	float inTempMem = dht.readTemperature();
	if ( isnan(inTempMem) ) 
	{
		Serial.println("Failed to read temperature from DHT sensor!");
		WebSerial.println("Failed to read temperature from DHT sensor!");
	}
	else
	{
		inTemp = inTempMem;
		Serial.print("DH22 Temp: "); Serial.print(inTemp); Serial.println();
		WebSerial.print("DH22 Temp: "); WebSerial.print(inTemp); WebSerial.println();
	}
	float inHumMem = dht.readHumidity();
	if ( isnan(inHumMem) ) 
	{
		Serial.println("Failed to read humidity from DHT sensor!");
		WebSerial.println("Failed to read humidity from DHT sensor!");
	}
	else
	{
		inHum = inHumMem;
		Serial.print("DH22 Humidity: "); Serial.print(inHum); Serial.println();
		WebSerial.print("DH22 Humidity: "); WebSerial.print(inHum); WebSerial.println();
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
	nightMode = EEPROM.read( nightModeFlashAddress );
	if (!nightMode || nightMode > 1)
	{
		nightMode = 0;
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
		String wifiStatus = "disconnected";
		if (wifiConnected)
		{
			wifiStatus = "connected";
		}
		String mqttStatus = "disconnected";
		if (mqttConnected)
		{
			mqttStatus = "connected";
		}		
		request->send(200, "text/html", "Room ID: <b><i>" + roomID + 
			"</i></b><br><br>Component ID: <b><i>" +  componentID + 
			"</i></b><br><br>Version: <b><i>" + String(_VERSION_) + 
			"</i></b><br><br>WiFi status: <b><i>" + wifiStatus  + 
			"</i></b><br><br>Mqtt status: <b><i>" + mqttStatus + "</i></b>");
	} );
	AsyncElegantOTA.begin(&server);    // Start ElegantOTA
	WebSerial.begin(&server);
	WebSerial.msgCallback(recvMsg);
	server.begin();
	Serial.println("HTTP server started");
	WebSerial.println("Component started with config " + roomID +  ":" + componentID);
	WebSerial.println("HTTP server started");
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
		if (isnan(RO))
		{
			Serial.println("No RO found for co2 mq135 sensor, please calibrate");
			WebSerial.println("No RO found for co2 mq135 sensor, please calibrate");
			RO = 1.0;
		}
		MQ135.setR0(RO);
	}
	if (USE_SGP30)
	{
		EEPROM.get(addressECO2, restoredECO2Baseline);
		EEPROM.get(addressTVOC, restoredTVOCBaseline); 
		Serial.print("Restored eCO2 baseline: ");
		Serial.println(restoredECO2Baseline,HEX);
		Serial.print("Restored TVOC baseline: ");
		Serial.println(restoredTVOCBaseline, HEX);
		Serial.println("SGP30 test");
		WebSerial.print("Restored eCO2 baseline: ");
		WebSerial.println(restoredECO2Baseline);
		WebSerial.print("Restored TVOC baseline: ");
		WebSerial.println(restoredTVOCBaseline);
		WebSerial.println("SGP30 test");
		if (!sgp.begin())
		{
			Serial.println("SGP30 Sensor not found :(");
			while (1);
		}
		Serial.print("Found SGP30 serial #");
		Serial.print(sgp.serialnumber[0], HEX);
		Serial.print(sgp.serialnumber[1], HEX);
		Serial.println(sgp.serialnumber[2], HEX);
		WebSerial.print("Found SGP30 serial #");
		WebSerial.print(sgp.serialnumber[0]);
		WebSerial.print(sgp.serialnumber[1]);
		WebSerial.println(sgp.serialnumber[2]);
		if (restoredECO2Baseline != 0) 
		{
			sgp.setIAQBaseline(restoredECO2Baseline, restoredTVOCBaseline);  
		}
	}
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