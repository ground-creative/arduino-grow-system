/**
	Grow system doser component
	Author: Ground Creative 
*/

#define _VERSION_ "1.0.3"
#include "doserDefaultConfig.h"
#include <NetTools.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <WebSerial.h>
#include <U8x8lib.h>

AsyncWebServer server(80);

TaskHandle_t netClient;
NetTools::WIFI network(ssid, password);

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

String mqttClientID = roomID + "-" + componentID;

bool wifiConnected = false, mqttConnected = false, oledOn = true, nightMode = false;
const int pumpOneFlashAddress = 0, pumpTwoFlashAddress = 10, pumpThreeFlashAddress = 20;
const int pumpFourFlashAddress = 30, pumpFiveFlashAddress = 40, pumpSixFlashAddress = 50;
const int nightModeFlashAddress = 70, oledFlashAddress = 90;
int pumpOneCal = 600, pumpTwoCal = 600, pumpThreeCal = 600;  // 1 mil 600 milliseconds;
int pumpFourCal = 600, pumpFiveCal = 600, pumpSixCal = 600;  // 1 mil 600 milliseconds;
int pOneCal, pTwoCal, pThreeCal, pFourCal, pFiveCal, pSixCal;
int pumpOneRelayState = HIGH, pumpTwoRelayState = HIGH, pumpThreeRelayState = HIGH;
int pumpFourRelayState = HIGH, pumpFiveRelayState = HIGH, pumpSixRelayState = HIGH;
unsigned long previousMillis = 0, displayPreviousMillis= 0; 
unsigned long checkConnectionInterval = 5000, updateInterval = 10000;
String wifiIP = "";

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
	//Serial.println("");
	WebSerial.print(content);
	if (String(topic) == roomID + "/" + componentID + "-restart")
	{
		ESP.restart();
	}
	else if (String(topic) == roomID + "/" + componentID + "-display-backlight")
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
			u8x8.clearDisplay();
		}
		EEPROM.write(oledFlashAddress, oledOn);
		EEPROM.commit();
	}
	else if (String(topic) == roomID + "/" + componentID + "-night-mode")
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
	else if (String(topic) == roomID + "/" + componentID + "/p-one")
	{
		Serial.println("IN");
		Serial.println(content.toInt()*pumpOneCal);
		digitalWrite(PUMP_ONE_RELAY_PIN, LOW); // turn on
		delay(content.toInt()*pumpOneCal);
		digitalWrite(PUMP_ONE_RELAY_PIN, HIGH);  // turn off
		Serial.println("OUT");
	}
	else if (String(topic) == roomID + "/" + componentID + "/p-two")
	{
		Serial.println("IN");
		Serial.println(content.toInt()*pumpTwoCal);
		digitalWrite(PUMP_TWO_RELAY_PIN, LOW); // turn on
		delay(content.toInt()*pumpTwoCal);
		digitalWrite(PUMP_TWO_RELAY_PIN, HIGH);  // turn off
		Serial.println("OUT");
	}
	else if (String(topic) == roomID + "/" + componentID + "/p-three")
	{
		Serial.println("IN");
		Serial.println(content.toInt()*pumpThreeCal);
		digitalWrite(PUMP_THREE_RELAY_PIN, LOW); // turn on
		delay(content.toInt()*pumpThreeCal);
		digitalWrite(PUMP_THREE_RELAY_PIN, HIGH);  // turn off
		Serial.println("OUT");
	}
	else if (String(topic) == roomID + "/" + componentID + "/p-four")
	{
		Serial.println("IN");
		Serial.println(content.toInt()*pumpFourCal);
		digitalWrite(PUMP_FOUR_RELAY_PIN, LOW); // turn on
		delay(content.toInt()*pumpFourCal);
		digitalWrite(PUMP_FOUR_RELAY_PIN, HIGH);  // turn off
		Serial.println("OUT");
	}
	else if (String(topic) == roomID + "/" + componentID + "/p-five")
	{
		Serial.println("IN");
		Serial.println(content.toInt()*pumpFiveCal);
		digitalWrite(PUMP_FIVE_RELAY_PIN, LOW); // turn on
		delay(content.toInt()*pumpFiveCal);
		digitalWrite(PUMP_FIVE_RELAY_PIN, HIGH);  // turn off
		Serial.println("OUT");
	}
	else if (String(topic) == roomID + "/" + componentID + "/p-six")
	{
		Serial.println("IN");
		Serial.println(content.toInt()*pumpSixCal);
		digitalWrite(PUMP_SIX_RELAY_PIN, LOW); // turn on
		delay(content.toInt()*pumpSixCal);
		digitalWrite(PUMP_SIX_RELAY_PIN, HIGH);  // turn off
		Serial.println("OUT");
	}
	else if (String(topic) == roomID + "/" + componentID + "/p-one-calibrate")
	{
		EEPROM.put(pumpOneFlashAddress, content.toInt());
		EEPROM.commit();
		pumpOneCal = content.toInt();
	}
	else if (String(topic) == roomID + "/" + componentID + "/p-two-calibrate")
	{
		EEPROM.put(pumpTwoFlashAddress, content.toInt());
		EEPROM.commit();
		pumpTwoCal = content.toInt();
	}
	else if (String(topic) == roomID + "/" + componentID + "/p-three-calibrate")
	{
		EEPROM.put(pumpThreeFlashAddress, content.toInt());
		EEPROM.commit();
		pumpThreeCal = content.toInt();
	}
	else if (String(topic) == roomID + "/" + componentID + "/p-four-calibrate")
	{
		EEPROM.put(pumpFourFlashAddress, content.toInt());
		EEPROM.commit();
		pumpFourCal = content.toInt();
	}
	else if (String(topic) == roomID + "/" + componentID + "/p-five-calibrate")
	{
		EEPROM.put(pumpFiveFlashAddress, content.toInt());
		EEPROM.commit();
		pumpFiveCal = content.toInt();
	}
	else if (String(topic) == roomID + "/" + componentID + "/p-six-calibrate")
	{
		EEPROM.put(pumpSixFlashAddress, content.toInt());
		EEPROM.commit();
		pumpSixCal = content.toInt();
	}
}

NetTools::MQTT mqtt(mqtt_server, mqtt_callback);

void mqttSubscribe(const String& roomID)
{
	Serial.println("Subscribing to mqtt messages");
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-night-mode").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-restart").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-one").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-two").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-three").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-four").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-five").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-six").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-one-calibrate").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-two-calibrate").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-three-calibrate").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-four-calibrate").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-five-calibrate").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-six-calibrate").c_str()));
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
	else if(d == "NIGHTMODE")
	{
		nightMode = v.toInt();
		if (nightMode)
		{
			Serial.println("Turning on night mode");
			WebSerial.println("Turning on night mode");
			digitalWrite(WIFI_LED_PIN, HIGH);
			digitalWrite(MQTT_LED_PIN, HIGH);
			mqtt.publish(const_cast<char*>(String("m/" + roomID + "/" + componentID + "-night-mode").c_str()), "1");
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
			mqtt.publish(const_cast<char*>(String("m/" + roomID + "/" + componentID + "-night-mode").c_str()), "0");
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
			mqtt.publish(const_cast<char*>(String("m/" + roomID + "/" + componentID + "-oled-on").c_str()), "1");
		}
		else
		{
			Serial.println("Turning off lcd");
			WebSerial.println("Turning off lcd");
			u8x8.clearDisplay();
			mqtt.publish(const_cast<char*>(String("m/" + roomID + "/" + componentID + "-oled-on").c_str()), "0");
		}
		EEPROM.write(oledFlashAddress, oledOn);
		EEPROM.commit();
	}
	else if(d == "GETCALVALUE")
	{
		int pump = v.toInt();
		if ( pump == 1 )
		{
			Serial.print("Calibration value pump "); Serial.print(pump); 
			Serial.print(":	"); Serial.print(pOneCal);
			WebSerial.print("Calibration value pump "); WebSerial.print(pump);
			WebSerial.print(":	"); WebSerial.print(pOneCal);
		}
		else if ( pump == 2 )
		{
			Serial.print("Calibration value pump "); Serial.print(pump); 
			Serial.print(":	"); Serial.print(pTwoCal);
			WebSerial.print("Calibration value pump "); WebSerial.print(pump);
			WebSerial.print(":	"); WebSerial.print(pTwoCal);
		}
		else if ( pump == 3 )
		{
			Serial.print("Calibration value pump "); Serial.print(pump); 
			Serial.print(":	"); Serial.print(pThreeCal);
			WebSerial.print("Calibration value pump "); WebSerial.print(pump);
			WebSerial.print(":	"); WebSerial.print(pThreeCal);
		}
		else if ( pump == 4 )
		{
			Serial.print("Calibration value pump "); Serial.print(pump); 
			Serial.print(":	"); Serial.print(pFourCal);
			WebSerial.print("Calibration value pump "); WebSerial.print(pump);
			WebSerial.print(":	"); WebSerial.print(pFourCal);
		}
		else if ( pump == 5 )
		{
			Serial.print("Calibration value pump "); Serial.print(pump); 
			Serial.print(":	"); Serial.print(pFiveCal);
			WebSerial.print("Calibration value pump "); WebSerial.print(pump);
			WebSerial.print(":	"); WebSerial.print(pFiveCal);
		}
		else if ( pump == 6 )
		{
			Serial.print("Calibration value pump "); Serial.print(pump); 
			Serial.print(":	"); Serial.print(pSixCal);
			WebSerial.print("Calibration value pump "); WebSerial.print(pump);
			WebSerial.print(":	"); WebSerial.print(pSixCal);
		}
	}
	else if(d == "CALVALUEPUMP1")
	{
		int pump = v.toInt();
		Serial.print("Setting calibration value for pump 1: ");  Serial.println(pump); 
		WebSerial.print("Setting calibration value for pump 1: ");  WebSerial.println(pump); 
		EEPROM.put(pumpOneFlashAddress, pump);
		EEPROM.commit();
		pumpOneCal = pump;
	}
	else if(d == "CALVALUEPUMP2")
	{
		int pump = v.toInt();
		Serial.print("Setting calibration value for pump 2: ");  Serial.println(pump); 
		WebSerial.print("Setting calibration value for pump 2: ");  WebSerial.println(pump); 
		EEPROM.put(pumpTwoFlashAddress, pump);
		EEPROM.commit();
		pumpTwoCal = pump;
	}
	else if(d == "CALVALUEPUMP3")
	{
		int pump = v.toInt();
		Serial.print("Setting calibration value for pump 3: ");  Serial.println(pump); 
		WebSerial.print("Setting calibration value for pump 3: ");  WebSerial.println(pump); 
		EEPROM.put(pumpThreeFlashAddress, pump);
		EEPROM.commit();
		pumpThreeCal = pump;
	}
	else if(d == "CALVALUEPUMP4")
	{
		int pump = v.toInt();
		Serial.print("Setting calibration value for pump 4: ");  Serial.println(pump); 
		WebSerial.print("Setting calibration value for pump 4: ");  WebSerial.println(pump); 
		EEPROM.put(pumpFourFlashAddress, pump);
		EEPROM.commit();
		pumpFourCal = pump;
	}
	else if(d == "CALVALUEPUMP5")
	{
		int pump = v.toInt();
		Serial.print("Setting calibration value for pump 5: ");  Serial.println(pump); 
		WebSerial.print("Setting calibration value for pump 5: ");  WebSerial.println(pump); 
		EEPROM.put(pumpFiveFlashAddress, pump);
		EEPROM.commit();
		pumpFiveCal = pump;
	}
	else if(d == "CALVALUEPUMP6")
	{
		int pump = v.toInt();
		Serial.print("Setting calibration value for pump 6: ");  Serial.println(pump); 
		WebSerial.print("Setting calibration value for pump 6: ");  WebSerial.println(pump); 
		EEPROM.put(pumpSixFlashAddress, pump);
		EEPROM.commit();
		pumpSixCal = pump;		
	}
	else if (d == "OPENPUMP1")
	{
		int pump = v.toInt();
		Serial.print("IN "); Serial.println(pump*pumpOneCal);
		WebSerial.print("IN "); WebSerial.println(pump*pumpOneCal);
		digitalWrite(PUMP_ONE_RELAY_PIN, LOW); // turn on
		delay(pump*pumpOneCal);
		digitalWrite(PUMP_ONE_RELAY_PIN, HIGH);  // turn off
		Serial.println("OUT");
		WebSerial.println("OUT");
	}
	else if (d == "OPENPUMP2")
	{
		int pump = v.toInt();
		Serial.print("IN "); Serial.println(pump*pumpTwoCal);
		WebSerial.print("IN "); WebSerial.println(pump*pumpTwoCal);
		digitalWrite(PUMP_TWO_RELAY_PIN, LOW); // turn on
		delay(pump*pumpTwoCal);
		digitalWrite(PUMP_TWO_RELAY_PIN, HIGH);  // turn off
		Serial.println("OUT");
		WebSerial.println("OUT");
	}
	else if (d == "OPENPUMP3")
	{
		int pump = v.toInt();
		Serial.print("IN "); Serial.println(pump*pumpThreeCal);
		WebSerial.print("IN "); WebSerial.println(pump*pumpThreeCal);
		digitalWrite(PUMP_THREE_RELAY_PIN, LOW); // turn on
		delay(pump*pumpThreeCal);
		digitalWrite(PUMP_THREE_RELAY_PIN, HIGH);  // turn off
		Serial.println("OUT");
		WebSerial.println("OUT");
	}
	else if (d == "OPENPUMP4")
	{
		int pump = v.toInt();
		Serial.print("IN "); Serial.println(pump*pumpFourCal);
		WebSerial.print("IN "); WebSerial.println(pump*pumpFourCal);
		digitalWrite(PUMP_FOUR_RELAY_PIN, LOW); // turn on
		delay(pump*pumpFourCal);
		digitalWrite(PUMP_FOUR_RELAY_PIN, HIGH);  // turn off
		Serial.println("OUT");
		WebSerial.println("OUT");
	}
	else if (d == "OPENPUMP5")
	{
		int pump = v.toInt();
		Serial.print("IN "); Serial.println(pump*pumpFiveCal);
		WebSerial.print("IN "); WebSerial.println(pump*pumpFiveCal);
		digitalWrite(PUMP_FIVE_RELAY_PIN, LOW); // turn on
		delay(pump*pumpFiveCal);
		digitalWrite(PUMP_FIVE_RELAY_PIN, HIGH);  // turn off
		Serial.println("OUT");
		WebSerial.println("OUT");
	}
	else if (d == "OPENPUMP6")
	{
		int pump = v.toInt();
		Serial.print("IN "); Serial.println(pump*pumpSixCal);
		WebSerial.print("IN "); WebSerial.println(pump*pumpSixCal);
		digitalWrite(PUMP_SIX_RELAY_PIN, LOW); // turn on
		delay(pump*pumpSixCal);
		digitalWrite(PUMP_SIX_RELAY_PIN, HIGH);  // turn off
		Serial.println("OUT");
		WebSerial.println("OUT");
	}
}

int changeRelayStateManually(int buttonPin, int relayPin, int &relayState)
{
	int buttonState = digitalRead(buttonPin); // read new state
	if (buttonState == LOW && relayState == HIGH) 
	{
		Serial.println("The button is being pressed");
		digitalWrite(relayPin, LOW); // turn on
		relayState = LOW;
	}
	else if (buttonState == HIGH && relayState == LOW) 
	{
		Serial.println("The button is unpressed");
		digitalWrite(relayPin, HIGH);  // turn off
		relayState = HIGH;
	}
	return relayState;
}

void netClientHandler( void * pvParameters )
{
	Serial.print("netClientHandler running on core ");
	Serial.println(xPortGetCoreID());
	for (;;)
	{
		unsigned long currentMillis = millis();
		if(!wifiConnected || currentMillis - previousMillis >= checkConnectionInterval) 
		{
			//Serial.println("Checking connection");
			previousMillis = currentMillis;
			if (!wifiConnected)
			{
				network.connect();
				digitalWrite(WIFI_LED_PIN, LOW);
				//u8x8.clearDisplay();
				if (oledOn)
				{
					//u8x8.drawString(0, 0, "Connected to WiFi");
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
				//u8x8.clearDisplay();
				if (oledOn)
				{
					//u8x8.drawString(0, 0, "Connected to WiFi");
				}
				wifiIP = network.localAddress().toString();
			}
			if (!mqtt.isConnected())
			{
				digitalWrite(MQTT_LED_PIN, HIGH);
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

void setInitParams()
{
	EEPROM.get(pumpOneFlashAddress,pOneCal);
	if ( pOneCal != -1)
	{
		pumpOneCal = pOneCal;
	}
	EEPROM.get(pumpTwoFlashAddress,pTwoCal);
	if ( pTwoCal != -1)
	{
		pumpTwoCal = pTwoCal;
	}
	EEPROM.get(pumpThreeFlashAddress,pThreeCal);
	if ( pThreeCal != -1)
	{
		pumpThreeCal = pThreeCal;
	}
	EEPROM.get(pumpFourFlashAddress,pFourCal);
	if ( pFourCal != -1)
	{
		pumpFourCal = pFourCal;
	}
	EEPROM.get(pumpFiveFlashAddress,pFiveCal);
	if ( pFiveCal != -1)
	{
		pumpFiveCal = pFiveCal;
	}
	EEPROM.get(pumpSixFlashAddress,pSixCal);
	if ( pSixCal != -1)
	{
		pumpSixCal = pSixCal;
	} 
} 

void setPinsInitStatus()
{
	pinMode(WIFI_LED_PIN, OUTPUT);
	digitalWrite(WIFI_LED_PIN, HIGH);
	pinMode(MQTT_LED_PIN, OUTPUT);
	digitalWrite(MQTT_LED_PIN, HIGH);
	pinMode(PUMP_ONE_RELAY_PIN, OUTPUT);
	digitalWrite(PUMP_ONE_RELAY_PIN, HIGH);
	pinMode(PUMP_ONE_BTN_PIN, INPUT_PULLUP);
	pinMode(PUMP_TWO_RELAY_PIN, OUTPUT);
	digitalWrite(PUMP_TWO_RELAY_PIN, HIGH);
	pinMode(PUMP_TWO_BTN_PIN, INPUT_PULLUP);
	pinMode(PUMP_THREE_RELAY_PIN, OUTPUT);
	digitalWrite(PUMP_THREE_RELAY_PIN, HIGH);
	pinMode(PUMP_THREE_BTN_PIN, INPUT_PULLUP);
	pinMode(PUMP_FOUR_RELAY_PIN, OUTPUT);
	digitalWrite(PUMP_FOUR_RELAY_PIN, HIGH);
	pinMode(PUMP_FOUR_BTN_PIN, INPUT_PULLUP);
	pinMode(PUMP_FIVE_RELAY_PIN, OUTPUT);
	digitalWrite(PUMP_FIVE_RELAY_PIN, HIGH);
	pinMode(PUMP_FIVE_BTN_PIN, INPUT_PULLUP);
	pinMode(PUMP_SIX_RELAY_PIN, OUTPUT);
	digitalWrite(PUMP_SIX_RELAY_PIN, HIGH);
	pinMode(PUMP_SIX_BTN_PIN, INPUT_PULLUP);
}

void updateDisplayValues()
{
	unsigned long currentMillis = millis();
	if ( currentMillis - displayPreviousMillis >= updateInterval ) 
	{
		displayPreviousMillis = currentMillis;
		u8x8.clearDisplay();
		u8x8.drawString(0, 0, wifiIP.c_str());
	}
}


void setup() 
{
	Serial.begin(9600);
	EEPROM.begin(255);
	Serial.println("Component started with config " + roomID +  ":" + componentID);
	setPinsInitStatus();
	setInitParams();
	//u8x8.begin();
	//u8x8.setFont(u8x8_font_chroma48medium8_r);
	//u8x8.drawString(0, 0, "Connected");
	//oledOn = EEPROM.read( oledFlashAddress );
	nightMode = EEPROM.read( nightModeFlashAddress );
	if (!nightMode || nightMode > 1)
	{
		nightMode = 0;
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
	});
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
	pumpOneRelayState = changeRelayStateManually(PUMP_ONE_BTN_PIN, PUMP_ONE_RELAY_PIN, pumpOneRelayState);
	pumpTwoRelayState = changeRelayStateManually(PUMP_TWO_BTN_PIN,PUMP_TWO_RELAY_PIN, pumpTwoRelayState);
	pumpThreeRelayState = changeRelayStateManually(PUMP_THREE_BTN_PIN,PUMP_THREE_RELAY_PIN, pumpThreeRelayState);
	pumpFourRelayState = changeRelayStateManually(PUMP_FOUR_BTN_PIN,PUMP_FOUR_RELAY_PIN, pumpFourRelayState);
	pumpFiveRelayState = changeRelayStateManually(PUMP_FIVE_BTN_PIN,PUMP_FIVE_RELAY_PIN, pumpFiveRelayState);
	pumpSixRelayState = changeRelayStateManually(PUMP_SIX_BTN_PIN,PUMP_SIX_RELAY_PIN, pumpSixRelayState);
	//updateDisplayValues();
}