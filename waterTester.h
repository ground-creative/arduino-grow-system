/**
	Grow system water tester component
	Author: Ground Creative 
*/

#define _VERSION_ "1.1.2"
#include "waterTesterDefaultConfig.h"
#include <NetTools.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <EEPROM.h>
#include "GravityTDS_ESP.h"
#include "DFRobot_ESP_PH.h"
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <WebSerial.h>

AsyncWebServer server(80);

GravityTDS_ESP gravityTds;
DFRobot_ESP_PH ph;
SSD1306AsciiWire oled;

// Component ID
String componentID = "water-tester";

TaskHandle_t netClient;
NetTools::WIFI network(ssid, password);

String mqttClientID = roomID + "-" + componentID;

OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

float voltage, phValue = 0.0, tdsValue = 0, waterTemp = 0.0, ecValue = 0.0;
unsigned int waterTempSensorCountRetries = 0;
unsigned long previousMillis = 0, netPreviousMillis = 0, updateInterval = 10000; 
bool wifiConnected = false, oledOn = true;
String wifiIP = "";

void recvMsg(uint8_t *data, size_t len)
{
	WebSerial.println("");
	WebSerial.println("Received Data...");
	String d = "";
	for(int i=0; i < len; i++)
	{
		d += char(data[i]);
	}
	WebSerial.println(d);
	if(d == "restart" || d == "RESTART")
	{
		ESP.restart();
	}
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{  
	Serial.println("");
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	WebSerial.println("");
	WebSerial.print("Message arrived [");
	WebSerial.print(topic);
	WebSerial.print("] ");
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
	else if (String(topic) == roomID + "/water-tester-display-backlight")
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
	else if (String(topic) == roomID + "/water-tester-display-update-interval")
	{
		updateInterval = content.toInt();
		Serial.print("Setting sensors update interval to ");
		Serial.print(updateInterval);
		WebSerial.print("Setting sensors update interval to ");
		WebSerial.print(String(updateInterval));
		EEPROM.put(updateIntervalFlashAddress, updateInterval);
		EEPROM.commit();
	}
}

NetTools::MQTT mqtt(mqtt_server, mqtt_callback);

void mqttSubscribe(const String& roomID)
{
	Serial.println("Subscribing to mqtt messages");
	WebSerial.println("Subscribing to mqtt messages");
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-backlight").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-update-interval").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-restart").c_str()));
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
	oled.println(String(phValue, 2));
	oled.setRow(4);
	oled.setCursor(30, 20);
	oled.println(String(waterTemp, 1) + "C");
	oled.setRow(6);
	oled.setCursor(30, 40);
	oled.println(String(tdsValue, 0) + "ppm");
}

void updateTds()
{
	if (!USE_TDS)
	{ 
		return; 
	}
	tdsValue = gravityTds.getTdsValue();  // then get the value
	ecValue = (tdsValue* 2/1000);
	Serial.print("tds:	");
	Serial.print(tdsValue,0);
	Serial.println("ppm");
	Serial.print("ec:	");
	Serial.println(ecValue,1);
	WebSerial.print("tds:	");
	WebSerial.print(String(tdsValue,0));
	WebSerial.println("ppm");
	WebSerial.print("ec:	");
	WebSerial.println(String(ecValue,1));
}

void updatePh()
{
	if (!USE_PH)
	{ 
		return; 
	}
	voltage = analogRead(PH_PIN) / ESPADC * ESPVOLTAGE; // read the voltage
	Serial.print("voltage:");
	Serial.println(voltage, 4);
	WebSerial.print("voltage:");
	WebSerial.println(String(voltage, 4));
	phValue = ph.readPH(voltage, waterTemp); // convert voltage to pH with temperature compensation
	Serial.print("pH:");
	Serial.println(phValue, 4);
	WebSerial.print("pH:");
	WebSerial.println(String(phValue, 4));
}

void updateTemp()
{
	if (!USE_TEMP)
	{ 
		return; 
	}
	sensors.requestTemperatures(); 
	float waterTempMem = sensors.getTempCByIndex(0);
	if (waterTempMem > -127.00)
	{
		waterTemp = waterTempMem;
		waterTempSensorCountRetries = 0;
	}
	else if (waterTempSensorCountRetries == waterTempSensorMaxRetries)
	{
		Serial.println("Cannot read temperature DS18B20 sensor problem, resetting");
		WebSerial.println("Cannot read temperature DS18B20 sensor problem, resetting");
		//delay( 1000 );
		ESP.restart();
	}
	else
	{      
		waterTempSensorCountRetries++;
		Serial.print("Cannot read temperature DS18B20, tried ");
		Serial.print(waterTempSensorCountRetries);
		Serial.print(" times");
		Serial.println();
		WebSerial.print("Cannot read temperature DS18B20, tried ");
		WebSerial.print(waterTempSensorCountRetries);
		WebSerial.print(" times");
		WebSerial.println();
	}
	Serial.print("DS18B20 Temp: ");
	Serial.print(waterTemp);
	Serial.println();
	WebSerial.print("DS18B20 Temp: ");
	WebSerial.print(waterTemp);
	WebSerial.println();
}

void publishValues()
{
	StaticJsonDocument<256> doc;
	doc["ppm"] = String( tdsValue, 0 );
	doc["ph"] = String( phValue, 2 );
	doc["ec"] = String( ecValue, 1 );
	doc["water_temp"] = String( waterTemp, 1);
	char buffer[256];
	serializeJson(doc, buffer);
	mqtt.publish(const_cast<char*>(String(roomID + "/water-tester").c_str()) , buffer);
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
	if (!updateInterval || updateInterval < 1000 || updateInterval > 4290000000)
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
	if (USE_TEMP){ sensors.begin( ); }
	if (USE_TDS)
	{
		gravityTds.setPin(TDS_PIN);
		gravityTds.setAref(ESPVOLTAGE/1000);
		gravityTds.setAdcRange(ESPADC);
		gravityTds.begin();  //initialization
	}
	if (USE_PH){ ph.begin(); }
	xTaskCreatePinnedToCore
	(
		netClientHandler,     /* Task function. */
		"netClient",          /* name of task. */
		10000,                /* Stack size of task */
		NULL,                 /* parameter of the task */
		1,                      /* priority of the task */
		&netClient,         /* Task handle to keep track of created task */
		0                     /* pin task to core 0 */   
	);                        
	delay(500); 
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) 
	{
		request->send(200, "text/plain", roomID + ":" + componentID + " " + " v" + String(_VERSION_) );
	} );
	AsyncElegantOTA.begin(&server);    // Start ElegantOTA
	WebSerial.begin(&server);
	WebSerial.msgCallback(recvMsg);
	server.begin();
	Serial.println("HTTP server started");
	WebSerial.println("Component started with config " + roomID +  ":" + componentID);
	WebSerial.println("HTTP server started");
}

void loop() 
{
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis >= updateInterval) 
	{
		previousMillis = currentMillis;
		updateTemp();
		updateTds();
		updatePh();
		updateDisplayValues(); 
		publishValues();
	}
	if (USE_TDS)
	{ 
		gravityTds.setTemperature(waterTemp); 
		gravityTds.update();  
	}
	if (USE_PH)
	{ 
		ph.calibration(voltage, waterTemp); // calibration process by Serial CMD
	}
}