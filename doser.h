/**
	Grow system doser component
	Author: Ground Creative 
	Version: 1.2
*/

#include "doserDefaultConfig.h"
#include <NetTools.h>
#include <Arduino.h>
#include <EEPROM.h>

TaskHandle_t netClient;
NetTools::WIFI network(ssid, password);

String mqttClientID = roomID + "-" + componentID;

bool wifiConnected = false;
const int pumpOneFlashAddress = 0, pumpTwoFlashAddress = 10, pumpThreeFlashAddress = 20;
const int pumpFourFlashAddress = 30, pumpFiveFlashAddress = 40, pumpSixFlashAddress = 50;
int pumpOneCal = 600,pumpTwoCal = 600,pumpThreeCal = 600;  // 1 mil 600 milliseconds;
int pumpFourCal = 600,pumpFiveCal = 600,pumpSixCal = 600;  // 1 mil 600 milliseconds;
int pOneCal,pTwoCal,pThreeCal,pFourCal,pFiveCal,pSixCal;
int pumpOneRelayState = HIGH, pumpTwoRelayState = HIGH, pumpThreeRelayState = HIGH;
int pumpFourRelayState = HIGH, pumpFiveRelayState = HIGH, pumpSixRelayState = HIGH;
unsigned long previousMillis = 0; 
unsigned long checkConnectionInterval = 5000;

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
	if (String(topic) == roomID + "/" + componentID + "-restart")
	{
		ESP.restart();
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
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-one").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-two").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-three").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-four").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-five").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-six").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-restart").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-one-calibrate").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-two-calibrate").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-three-calibrate").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-four-calibrate").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-five-calibrate").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/p-six-calibrate").c_str()));
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
			}
			else if (network.status() != WL_CONNECTED)
			{
				digitalWrite(WIFI_LED_PIN, HIGH);
				digitalWrite(MQTT_LED_PIN, HIGH);
				network.check();
				while (network.status( ) != WL_CONNECTED)
				{
					Serial.print('.');
					delay(1000);
				}
				digitalWrite(WIFI_LED_PIN, LOW);
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

void setup() 
{
	Serial.begin(9600);
	EEPROM.begin(255);
	Serial.println("Component started with config " + roomID +  ":" + componentID);
	setPinsInitStatus();
	setInitParams();
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
}

void loop() 
{
	pumpOneRelayState = changeRelayStateManually(PUMP_ONE_BTN_PIN, PUMP_ONE_RELAY_PIN, pumpOneRelayState);
	pumpTwoRelayState = changeRelayStateManually(PUMP_TWO_BTN_PIN,PUMP_TWO_RELAY_PIN, pumpTwoRelayState);
	pumpThreeRelayState = changeRelayStateManually(PUMP_THREE_BTN_PIN,PUMP_THREE_RELAY_PIN, pumpThreeRelayState);
	pumpFourRelayState = changeRelayStateManually(PUMP_FOUR_BTN_PIN,PUMP_FOUR_RELAY_PIN, pumpFourRelayState);
	pumpFiveRelayState = changeRelayStateManually(PUMP_FIVE_BTN_PIN,PUMP_FIVE_RELAY_PIN, pumpFiveRelayState);
	pumpSixRelayState = changeRelayStateManually(PUMP_SIX_BTN_PIN,PUMP_SIX_RELAY_PIN, pumpSixRelayState);
}