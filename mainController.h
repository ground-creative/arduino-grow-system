/**
	Grow system main controller component
	Author: Ground Creative 
*/

#define _VERSION_ "1.4.2"
#include "mainControllerDefaultConfig.h"
#include <NetTools.h>
#include <Preferences.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "Arduino.h"
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <WebSerial.h>

AsyncWebServer server(80);

TaskHandle_t netClient;
Preferences preferences;
NetTools::WIFI network(ssid, password);

LiquidCrystal_I2C lcd(lcdAddress, lcdColumns, lcdRows);

// Variables
String componentID = "main-controller";
String mqttClientID = roomID + "-" + componentID;
float water_temp = 0.0, ph = 0.0, ec = 0.0, ppm = 0, co2Val = 0, t = 0.0, h = 0.0, dtemperature = 0.0;
int water_level = 0, solValveBtnState, drainPumpBtnState, mixingPumpBtnState;
int extractorBtnState, lightsBtnState, feedingPumpBtnState, fanBtnState, aircoBtnState;
int solValveRelayState = HIGH, drainPumpRelayState = HIGH, mixingPumpRelayState = HIGH, extractorRelayState = HIGH;
int lightsRelayState = HIGH, feedingPumpRelayState = HIGH, fanRelayState = HIGH, aircoRelayState = HIGH;
int resetButtonState = HIGH; 
long lastDebounceTime = 0, debounceDelay = 200;
unsigned long previousMillis = 0, netPreviousMillis = 0, updateInterval;
bool wifiConnected = false, backlightOn;

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

boolean debounceButton(boolean state, const int pin)
{
	boolean stateNow = digitalRead(pin);
	if(state != stateNow)
	{
		delay(50);
		stateNow = digitalRead(pin);
	}
	return stateNow;
}

void changeRelayState(int value, int relayPin, const char* prefKey)
{
	Serial.println("");
	WebSerial.println("");
	if(value)
	{
		Serial.println("Switching on relay");
		WebSerial.println("Switching on relay");
		digitalWrite(relayPin, LOW);
		preferences.putInt(prefKey, 0);
	} 
	else
	{
		Serial.println("Switching off relay");
		WebSerial.println("Switching off relay");
		digitalWrite(relayPin, HIGH);
		preferences.putInt(prefKey, 1);
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
	else if (String(topic) == roomID + "/main-controller-display-backlight")
	{
		backlightOn = content.toInt();
		if (backlightOn)
		{
			Serial.println("Turning on lcd");
			WebSerial.println("Turning on lcd");
			lcd.backlight();
		}
		else
		{
			Serial.println("Turning off lcd");
			WebSerial.println("Turning off lcd");
			lcd.noBacklight();
		}
		preferences.putInt("backlight-on", backlightOn);
	}
	else if(String(topic) == roomID + "/main-controller-display-update-interval")
	{
		updateInterval = content.toInt();
		Serial.print("Setting display update interval to ");
		Serial.print(updateInterval);
		WebSerial.print("Setting display update interval to ");
		WebSerial.print(String(updateInterval));
		preferences.putUInt("update-interval", updateInterval);
	}
	else if (String(topic) == roomID + "/air-sensors")
	{
		StaticJsonDocument<500> doc;
		deserializeJson(doc,  content);
		float tempDtemperature = doc["o"];
		if ( tempDtemperature > 0 )
		{
			dtemperature = tempDtemperature;
		}
		float tempT = doc["i"];
		if ( tempT> 0 )
		{
			t = tempT;
		}
		float tempH = doc["h"];
		if ( tempH > 0 )
		{
			h = tempH;
		}
		float tempCo2Val = doc["c"];
		if ( tempCo2Val > 0 )
		{
			co2Val = tempCo2Val;
		}
	}
	else if (String(topic) == roomID + "/water-level")
	{
		StaticJsonDocument<500> doc;
		deserializeJson(doc,  content);
		water_level = doc["level"];
		int tempWater_level = doc["level"];
		if ( tempWater_level > 0 )
		{
			water_level = tempWater_level;
		}
	}
	else if (String(topic) == roomID + "/water-tester")
	{
		StaticJsonDocument<500> doc;
		deserializeJson(doc,  content);
		float tempWater_temp = doc["water_temp"];
		if ( tempWater_temp > 0 )
		{
			water_temp = tempWater_temp;
		}
		float tempPH = doc["ph"];
		if ( tempPH > 0 )
		{
			ph = tempPH;
		}
		float tempPPM = doc["ppm"];
		if ( tempPPM > 0 )
		{
			ppm = tempPPM;
		}
		float tempEC = doc["ec"];
		if ( tempEC > 0 )
		{
			ec = tempEC;
		}
	}
	else if (String(topic) == roomID + "/water-valve")
	{
		changeRelayState(content.toInt(), SOLENOID_VALVE_RELAY_PIN, "solenoid-valve");
	}
	else if (String(topic) == roomID + "/drain-pump")
	{
		changeRelayState(content.toInt(), DRAIN_PUMP_RELAY_PIN, "drain-pump");
	}
	else if (String(topic) == roomID + "/mixing-pump")
	{
		changeRelayState(content.toInt(), MIXING_PUMP_RELAY_PIN, "mixing-pump");
	}
	else if (String(topic) == roomID + "/extractor")
	{
		changeRelayState(content.toInt(), EXTRACTOR_RELAY_PIN, "extractor");
	}
	else if (String(topic) == roomID + "/lights")
	{
		changeRelayState(content.toInt(), LIGHTS_RELAY_PIN, "lights");
	}
	else if (String(topic) == roomID + "/feeding-pump")
	{
		changeRelayState(content.toInt(), FEEDING_PUMP_RELAY_PIN, "feeding-pump");
	}
	else if (String(topic) == roomID + "/fan")
	{
		changeRelayState(content.toInt(), FAN_RELAY_PIN, "fan");
	}
	else if (String(topic) == roomID + "/airco")
	{
		changeRelayState(content.toInt(), AIRCO_RELAY_PIN, "airco");
	}
}

NetTools::MQTT mqtt(mqtt_server, mqtt_callback);

void mqttSubscribe(const String& roomID)
{
	Serial.println("Subscribing to mqtt messages");
	WebSerial.println("Subscribing to mqtt messages");
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-backlight").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-restart").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/water-tester").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/water-level").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/drain-pump").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/water-valve").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/feeding-pump").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/mixing-pump").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/extractor").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/lights").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/fan").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/airco").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/air-sensors").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-update-interval").c_str()));
}

int changeRelayStateManually(int curBtnState, int &relayState, int btnPin, int relayPin, const char* prefKey, String mqttTopic = "")
{
	int lastButtonState = curBtnState;
	curBtnState = debounceButton(relayState, btnPin);
	if(lastButtonState == HIGH && curBtnState == LOW) 
	{
		Serial.println();
		Serial.print("The button is pressed ");
		Serial.println(mqttTopic);
		WebSerial.println();
		WebSerial.print("The button is pressed ");
		WebSerial.println(mqttTopic);
		relayState = !relayState;
		digitalWrite(relayPin, relayState);
		preferences.putInt(prefKey, relayState);
		mqtt.publish(const_cast<char*>(mqttTopic.c_str()), const_cast<char*>(String("LOW" == (relayState ? "HIGH" : "LOW")).c_str()));
	}
	return curBtnState;
}

void updateDisplayValues()
{
	unsigned long currentMillis = millis();
	if ( currentMillis - previousMillis >= updateInterval ) 
	{
		previousMillis = currentMillis;
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print(String(dtemperature, 1).c_str());
		lcd.print(( char )223 );
		lcd.print(" | ");
		lcd.print(String( t, 1 ).c_str());
		lcd.print((char) 223);
		String valH = String(h, 1).c_str();
		int lenH = valH.length();
		int hCursorPos = 15;
		if (lenH == 3)
		{
			hCursorPos = 16;
		}
		lcd.setCursor(hCursorPos, 0);
		lcd.print(valH);
		lcd.print("%");
		lcd.setCursor( 4, 1 );
		lcd.print("Water & Co2");
		lcd.setCursor(0, 2);
		lcd.print("ph:");
		lcd.print(String(ph, 2).c_str());
		lcd.setCursor(8, 2);
		lcd.print(String(water_temp, 1).c_str());
		lcd.print((char) 223);
		String valCo2 = String(co2Val, 0).c_str();
		int lenCo2 = valCo2.length();
		int co2CursorPos = 16;
		if (lenCo2 == 4)
		{
			co2CursorPos = 13;
		}
		if (lenCo2 == 3)
		{
			co2CursorPos = 14;
		}
		if (lenCo2 == 2)
		{
			co2CursorPos = 15;
		}
		lcd.setCursor(co2CursorPos, 2);
		lcd.print(valCo2);
		lcd.print("ppm");
		lcd.setCursor(9, 3);
		lcd.print(water_level);
		lcd.print("%");
		lcd.setCursor(0, 3);
		lcd.print("ec:");
		lcd.print(String(ec, 1).c_str());
		String valPPM = String(ppm, 0).c_str();
		int lenPPM = valPPM.length();
		int ppmCursorPos = 16;
		if (lenPPM == 4)
		{
			ppmCursorPos = 13;
		}
		else if (lenPPM == 3)
		{
			ppmCursorPos = 14;
		}
		else if (lenPPM == 2)
		{
			ppmCursorPos = 15;
		}
		lcd.setCursor(ppmCursorPos, 3);
		lcd.print(valPPM);
		lcd.print("ppm");
	}
}

void lcdBacklightBtn()
{
	resetButtonState = digitalRead(LCD_PIN);
	if ( (millis() - lastDebounceTime) > debounceDelay ) 
	{
		if ( (resetButtonState == LOW) && (backlightOn) ) 
		{
			Serial.println("Turning off lcd");
			WebSerial.println("Turning off lcd");
			backlightOn = 0;
			lcd.noBacklight();
			lastDebounceTime = millis();
			preferences.putInt("backlight-on", backlightOn);
		}
		else if ( (resetButtonState == LOW) ) 
		{
			Serial.println("Turning on lcd");
			WebSerial.println("Turning on lcd");
			backlightOn = 1;
			lcd.backlight();
			lastDebounceTime = millis();
			preferences.putInt("backlight-on", backlightOn);

		}
	}
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
				digitalWrite(WIFI_LED_PIN, HIGH);
				lcd.clear();
				lcd.setCursor(0, 0);
				lcd.print("Connected to WiFi");
				lcd.setCursor(0, 1);
				lcd.print("ip: ");
				lcd.setCursor(4, 1);
				lcd.print(network.localAddress().toString());
			}
			else if (network.status() != WL_CONNECTED)
			{
				digitalWrite(WIFI_LED_PIN, LOW);
				digitalWrite(MQTT_LED_PIN, LOW);
				network.check();
				while (network.status( ) != WL_CONNECTED)
				{
					Serial.print('.');
					delay(1000);
				}
				digitalWrite(WIFI_LED_PIN, HIGH);
				lcd.clear();
				lcd.setCursor(0, 0);
				lcd.print("Connected to WiFi");
				lcd.setCursor(0, 1);
				lcd.print("ip: ");
				lcd.setCursor(4, 1);
				lcd.print(network.localAddress().toString());
			}
			if (!mqtt.isConnected())
			{
				digitalWrite(MQTT_LED_PIN, LOW);
				if (mqtt.connect(mqttClientID, mqtt_username, mqtt_password))
				{
					mqttSubscribe(roomID);
					digitalWrite(MQTT_LED_PIN, HIGH);
					delay(1000);
				}
			}
			wifiConnected = true;
		}
		mqtt.loop();
		delay(100);
	}
}

void setup() 
{
	// Serial
	Serial.begin(9600);
	// Preferences
	preferences.begin("relays", false);
	//delay(500);
	//while ( !Serial ){ ; }         // wait for serial port to connect. Needed for native USB port only
	Serial.println("Component started with config " + roomID +  ":" + componentID);
	pinMode(WIFI_LED_PIN, OUTPUT);
	digitalWrite(WIFI_LED_PIN, LOW);
	pinMode(MQTT_LED_PIN, OUTPUT);
	digitalWrite(MQTT_LED_PIN, LOW);
	// Set pin mode solenoid valve
	pinMode(SOLENOID_VALVE_RELAY_PIN, OUTPUT);
	pinMode(SOLENOID_VALVE_BTN_PIN, INPUT_PULLUP);
	// Set pin mode drain pump
	pinMode(DRAIN_PUMP_RELAY_PIN, OUTPUT);
	pinMode(DRAIN_PUMP_BTN_PIN, INPUT_PULLUP);
	// Set pin mode mixing pump
	pinMode(MIXING_PUMP_RELAY_PIN, OUTPUT);
	pinMode(MIXING_PUMP_BTN_PIN, INPUT_PULLUP);
	// Set pin mode extractor
	pinMode(EXTRACTOR_RELAY_PIN, OUTPUT);
	pinMode(EXTRACTOR_BTN_PIN, INPUT_PULLUP);	
	// Set pin mode lights
	pinMode(LIGHTS_RELAY_PIN, OUTPUT );
	pinMode(LIGHTS_BTN_PIN, INPUT_PULLUP);
	// Set pin mode feeding pump
	pinMode(FEEDING_PUMP_RELAY_PIN, OUTPUT);
	pinMode(FEEDING_PUMP_BTN_PIN, INPUT_PULLUP);
	// Set pin mode fan
	pinMode(FAN_RELAY_PIN, OUTPUT);
	pinMode(FAN_BTN_PIN, INPUT_PULLUP);
	// Set pin mode airco
	pinMode(AIRCO_RELAY_PIN, OUTPUT);
	pinMode(AIRCO_BTN_PIN, INPUT_PULLUP);
	// Set pin mode reset button
	pinMode(LCD_PIN, INPUT_PULLUP);
	// Solenoid valve
	solValveRelayState = preferences.getInt("solenoid-valve", 1);
	digitalWrite(SOLENOID_VALVE_RELAY_PIN, solValveRelayState );
	// Drain pump
	drainPumpRelayState = preferences.getInt("drain-pump", 1);
	digitalWrite(DRAIN_PUMP_RELAY_PIN, drainPumpRelayState);
	// Mixing pump
	mixingPumpRelayState = preferences.getInt("mixing-pump", 1);
	digitalWrite(MIXING_PUMP_RELAY_PIN, mixingPumpRelayState);
	// Extractor
	extractorRelayState = preferences.getInt("extractor", 1);
	digitalWrite(EXTRACTOR_RELAY_PIN, extractorRelayState);		
	// Lights
	lightsRelayState = preferences.getInt("lights", 1);
	digitalWrite(LIGHTS_RELAY_PIN, lightsRelayState);
	// Feeding pump
	feedingPumpRelayState = preferences.getInt("feeding-pump", 1);
	digitalWrite(FEEDING_PUMP_RELAY_PIN, feedingPumpRelayState);
	// Fan
	fanRelayState = preferences.getInt("fan", 1);
	digitalWrite(FAN_RELAY_PIN, fanRelayState);
	// Airco
	aircoRelayState = preferences.getInt("airco", 1);
	digitalWrite(AIRCO_RELAY_PIN, aircoRelayState);
	// Lcd
	Wire.begin(); 
	lcd.begin();
	lcd.clear();
	lcd.setCursor(0, 0); 
	lcd.print("Starting"); 
	lcd.setCursor(0, 2); 
	lcd.print("roomID: " + roomID); 
	updateInterval = preferences.getULong("update-interval", 10000);
	backlightOn = preferences.getInt("backlight-on", 1);
	if (backlightOn)
	{
		lcd.backlight();
	}
	else
	{
		lcd.noBacklight();
	}
	xTaskCreatePinnedToCore
	(
		netClientHandler,   	/* Task function. */
		"netClient",        	/* name of task. */
		10000,              	/* Stack size of task */
		NULL,               	/* parameter of the task */
		1,                  		/* priority of the task */
		&netClient,        	/* Task handle to keep track of created task */
		0                  		/* pin task to core 0 */   
	);                        
	delay(500); 
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) 
	{
		request->send(200, "text/plain", roomID + ":" + componentID + " " + " v" + String(_VERSION_));
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
	solValveBtnState = changeRelayStateManually(solValveBtnState, solValveRelayState, SOLENOID_VALVE_BTN_PIN, SOLENOID_VALVE_RELAY_PIN, "solenoid-valve", "m/" + roomID + "/sv-btn");
	drainPumpBtnState = changeRelayStateManually(drainPumpBtnState, drainPumpRelayState, DRAIN_PUMP_BTN_PIN, DRAIN_PUMP_RELAY_PIN, "drain-pump","m/" + roomID + "/dp-btn");
	mixingPumpBtnState = changeRelayStateManually(mixingPumpBtnState, mixingPumpRelayState, MIXING_PUMP_BTN_PIN, MIXING_PUMP_RELAY_PIN, "mixing-pump", "m/" + roomID + "/mp-btn");
	extractorBtnState = changeRelayStateManually(extractorBtnState, extractorRelayState, EXTRACTOR_BTN_PIN, EXTRACTOR_RELAY_PIN, "extractor", "m/" + roomID + "/ex-btn");
	lightsBtnState = changeRelayStateManually(lightsBtnState, lightsRelayState, LIGHTS_BTN_PIN, LIGHTS_RELAY_PIN, "lights", "m/" + roomID + "/li-btn");
	feedingPumpBtnState = changeRelayStateManually(feedingPumpBtnState, feedingPumpRelayState, FEEDING_PUMP_BTN_PIN, FEEDING_PUMP_RELAY_PIN, "feeding-pump", "m/" + roomID + "/fp-btn");
	fanBtnState = changeRelayStateManually(fanBtnState, fanRelayState, FAN_BTN_PIN, FAN_RELAY_PIN, "fan", "m/" + roomID + "/fa-btn");
	aircoBtnState = changeRelayStateManually(aircoBtnState, aircoRelayState, AIRCO_BTN_PIN, AIRCO_RELAY_PIN, "airco", "m/" + roomID + "/ai-btn");
	lcdBacklightBtn();
	updateDisplayValues();
}