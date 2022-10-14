/**
	Grow system main controller component
	Author: Carlo Pietrobattista
	Version: 1.0
*/

#ifndef MAIN_CONTROLLER_CONFIG
	#include "mainControllerDefaultConfig.h"
#endif
#include <NetTools.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "Arduino.h"
#include <ArduinoJson.h>
#include "SoftwareSerial.h"

LiquidCrystal_I2C lcd(lcdAddress, lcdColumns, lcdRows);
SoftwareSerial nodemcu(RXD2, TXD2); //RX, TX

// Variables
float water_temp = 0.0, ph = 0.0, ec = 0.0, ppm = 0, co2Val = 0, t = 0.0, h = 0.0, dtemperature = 0.0;
int water_level = 0, solValveBtnState, drainPumpBtnState, mixingPumpBtnState, extractorBtnState, lightsBtnState, feedingPumpBtnState, fanBtnState, aircoBtnState;
unsigned long previousMillis = 0;
//String componentID = "main-controller";

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

void sendSerialData(String dataType, String message, String payload)
{
	StaticJsonDocument<500> doc;
	doc["t"] = dataType;
	doc["m"] = message;
	doc["p"] = payload;
	Serial.println("Sending serial data: ");
	serializeJson(doc, Serial);
	serializeJson(doc, nodemcu);
	nodemcu.print('\n');
}

void changeRelayState(int value, int relayPin, int flashAddress)
{
	if(value)
	{
		Serial.println("Switching on relay");
		digitalWrite(relayPin, LOW);
		EEPROM.write(flashAddress, 0);
	} 
	else
	{
		Serial.println("Switching off relay");
		digitalWrite(relayPin, HIGH);
		EEPROM.write(flashAddress, 1);
	}    
	EEPROM.commit();
}

int changeRelayStateManually(int curBtnState, int &relayState, int btnPin, int relayPin, int flashAddress, String mqttTopic = "")
{
	int lastButtonState = curBtnState;
	curBtnState = debounceButton(relayState, btnPin);
	if(lastButtonState == HIGH && curBtnState == LOW) 
	{
		Serial.println();
		Serial.print("The button is pressed ");
		Serial.println(mqttTopic);
		relayState = !relayState;
		digitalWrite(relayPin, relayState);
		EEPROM.write(flashAddress, relayState);
		EEPROM.commit();
		sendSerialData("m", mqttTopic, String("LOW" == (relayState ? "HIGH" : "LOW")));
	}
	return curBtnState;
}

void processSerialCommand(String message, String payload)
{
	if (message == roomID + "/water-valve")
	{
		changeRelayState(payload.toInt(), SOLENOID_VALVE_RELAY_PIN, SOLENOID_VALVE_BTN_PIN);
	}
	else if (message == roomID + "/drain-pump")
	{
		changeRelayState(payload.toInt(), DRAIN_PUMP_RELAY_PIN, DRAIN_PUMP_BTN_PIN);
	}
	else if (message == roomID + "/mixing-pump")
	{
		changeRelayState(payload.toInt(), MIXING_PUMP_RELAY_PIN, MIXING_PUMP_BTN_PIN);
	}
	else if (message == roomID + "/extractor")
	{
		changeRelayState(payload.toInt(), EXTRACTOR_RELAY_PIN, EXTRACTOR_BTN_PIN);
	}
	else if (message == roomID + "/lights")
	{
		changeRelayState(payload.toInt(), LIGHTS_RELAY_PIN, LIGHTS_BTN_PIN);
	}
	else if (message == roomID + "/feeding-pump")
	{
		changeRelayState(payload.toInt(), FEEDING_PUMP_RELAY_PIN, FEEDING_PUMP_BTN_PIN);
	}
	else if (message == roomID + "/fan")
	{
		changeRelayState(payload.toInt(), FAN_RELAY_PIN, FAN_BTN_PIN);
	}
	else if (message == roomID + "/airco")
	{
		changeRelayState(payload.toInt(), AIRCO_RELAY_PIN, AIRCO_BTN_PIN);
	}
	else if (String(message ) == "display-backlight")
	{
		backlightOn = payload.toInt();
		EEPROM.write(backlightOnFlashAddress, backlightOn);
		EEPROM.commit();
		if (backlightOn)
		{
			Serial.println("Turning on lcd");
			lcd.backlight();
		}
		else
		{
			Serial.println("Turning off lcd");
			lcd.noBacklight();
		}
	}
	else	if (String( message ) == "restart")
	{
	      ESP.restart( );
	}
	else if (String( message ) == "config-component-id")
	{
		Serial.println("Sending config data to wifi chip");
		sendSerialData("j", "c", "{\"componentID\":\"main-controller\"}");
	}
	else
	{
		Serial.println("Coudn't process serial command");
	}
}

void readSerialData()
{
	if (nodemcu.available() > 0) 
	{
		String content = "";
		while (nodemcu.available() > 0) 
		{
			content += nodemcu.readString();
		}
		StaticJsonDocument<500> doc;
		deserializeJson(doc, content);
		const char* msgType = doc["t"];
		const char* message = doc["m"];
		const char* payload = doc["p"];
		Serial.println("");
		if(String(msgType) == "i")
		{
			Serial.println("Received serial info data");
			lcd.clear( );
			lcd.setCursor ( 0, 0 ); 
			lcd.print( message ); 
			lcd.setCursor ( 0, 1 ); 
			lcd.print( payload );
		}
		else if(String(msgType) == "c")
		{
			Serial.println("Received serial command");
			processSerialCommand(message, payload);
		}
		else if(String(msgType) == "j")
		{
			Serial.println("Received serial json data");
			StaticJsonDocument<500> doc1;
			deserializeJson(doc1, doc["p"]);
			if(String(message) == "air")
			{
				float tempDtemperature = doc1["o"];
				if ( tempDtemperature > 0 )
				{
					dtemperature = tempDtemperature;
				}
				float tempT = doc1["i"];
				if ( tempT> 0 )
				{
					t = tempT;
				}
				h = doc1["h"];
				float tempH = doc1["h"];
				if ( tempH > 0 )
				{
					h = tempH;
				}
				float tempCo2Val = doc1["c"];
				if ( tempCo2Val > 0 )
				{
					co2Val = tempCo2Val;
				}
			}
			if(String(message) == "wl")
			{
				water_level = doc1["level"];
				int tempWater_level = doc1["level"];
				if ( tempWater_level > 0 )
				{
					water_level = tempWater_level;
				}
			}
			if(String(message ) == "display-update-interval")
			{
				updateInterval = doc1[ "interval" ];
				EEPROM.writeFloat(updateIntervalFlashAddress, updateInterval);
				EEPROM.commit();
				Serial.print("Setting dipslay update interval to");
				Serial.print(updateInterval);
				Serial.println();
			}
			if ( String(message) == roomID + "/" + "water-tester" ) // to be changed
			{
				float tempWater_temp = doc1["water_temp"];
				if ( tempWater_temp > 0 )
				{
					water_temp = tempWater_temp;
				}
				float tempPH = doc1["ph"];
				if ( tempPH > 0 )
				{
					ph = tempPH;
				}
				float tempPPM = doc1["ppm"];
				if ( tempPPM > 0 )
				{
					ppm = tempPPM;
				}
				float tempEC = doc1["ec"];
				if ( tempEC > 0 )
				{
					ec = tempEC;
				}
			}
		}
		else
		{
			Serial.println("Received serial uncaught data");
		}
		Serial.println(content);
	}
}

void updateDisplayValues()
{
	unsigned long currentMillis = millis();
	if ( currentMillis - previousMillis >= updateInterval ) 
	{
		previousMillis = currentMillis;
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print( String(dtemperature, 1).c_str() );
		lcd.print( ( char )223 );
		lcd.print(" | ");
		lcd.print( String( t, 1 ).c_str() );
		lcd.print( ( char ) 223 );
		String valH = String(h, 1 ).c_str();
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
		lcd.print( "Water & Co2" );
		lcd.setCursor( 0, 2 );
		lcd.print( "ph:" );
		lcd.print( String( ph, 1 ).c_str() );
		lcd.setCursor( 8, 2);
		lcd.print( String( water_temp, 1 ).c_str( ) );
		lcd.print( ( char ) 223 );
		String valCo2 = String( co2Val, 0 ).c_str( );
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
		lcd.setCursor( co2CursorPos, 2 );
		lcd.print( valCo2 );
		lcd.print( "ppm" );
		lcd.setCursor( 9, 3 );
		lcd.print( water_level );
		lcd.print( "%" );
		lcd.setCursor( 0, 3 );
		lcd.print( "ec:" );
		lcd.print( String( ec, 1 ).c_str( ) );
		String valPPM = String( ppm, 0 ).c_str( );
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
		lcd.setCursor( ppmCursorPos, 3 );
		lcd.print( valPPM );
		lcd.print( "ppm" );
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
			backlightOn = 0;
			lcd.noBacklight();
			lastDebounceTime = millis();
			EEPROM.write(backlightOnFlashAddress, backlightOn);
			EEPROM.commit();
			sendSerialData("m", "m/" + roomID + "/backlight-btn", String(backlightOn));
		}
		else if ( (resetButtonState == LOW) ) 
		{
			Serial.println("Turning on lcd");
			backlightOn = 1;
			lcd.backlight();
			lastDebounceTime = millis();
			EEPROM.write(backlightOnFlashAddress, backlightOn);
			EEPROM.commit();
			sendSerialData("m", "m/" + roomID + "/backlight-btn", String(backlightOn));
		}
	}
}

void setup() 
{
	// Serial
	Serial.begin(9600);
	delay(1000);
	//while ( !Serial ){ ; }         // wait for serial port to connect. Needed for native USB port only
	// Serial2
	nodemcu.begin(BAUD_RATE);
	Serial.println( "Starting" );
	// Set pin mode  solenoid valve
	pinMode( SOLENOID_VALVE_RELAY_PIN, OUTPUT );
	pinMode( SOLENOID_VALVE_BTN_PIN, INPUT_PULLUP );
	// Set pin mode  drain pump
	pinMode( DRAIN_PUMP_RELAY_PIN, OUTPUT );
	pinMode( DRAIN_PUMP_BTN_PIN, INPUT_PULLUP );
	// Set pin mode  mixing pump
	pinMode( MIXING_PUMP_RELAY_PIN, OUTPUT );
	pinMode( MIXING_PUMP_BTN_PIN, INPUT_PULLUP );
	// Set pin mode  extractor
	pinMode( EXTRACTOR_RELAY_PIN, OUTPUT );
	pinMode( EXTRACTOR_BTN_PIN, INPUT_PULLUP );	
	// Set pin mode lights
	pinMode( LIGHTS_RELAY_PIN, OUTPUT );
	pinMode( LIGHTS_BTN_PIN, INPUT_PULLUP );
	// Set pin mode feeding pump
	pinMode( FEEDING_PUMP_RELAY_PIN, OUTPUT );
	pinMode( FEEDING_PUMP_BTN_PIN, INPUT_PULLUP );
	// Set pin mode fan
	pinMode( FAN_RELAY_PIN, OUTPUT );
	pinMode( FAN_BTN_PIN, INPUT_PULLUP );
	// Set pin mode airco
	pinMode( AIRCO_RELAY_PIN, OUTPUT );
	pinMode( AIRCO_BTN_PIN, INPUT_PULLUP );
	// Set pin mode reset button
	pinMode(LCD_PIN, INPUT_PULLUP );
	// EEPROM
	EEPROM.begin( 255 );
	// Solenoid valve
	solValveRelayState = EEPROM.read( solValveFlashAddress );
	digitalWrite( SOLENOID_VALVE_RELAY_PIN, solValveRelayState );
	// Drain pump
	drainPumpRelayState = EEPROM.read( drainPumpFlashAddress );
	digitalWrite( DRAIN_PUMP_RELAY_PIN, drainPumpRelayState );
	// Mixing pump
	mixingPumpRelayState = EEPROM.read( mixingPumpFlashAddress );
	digitalWrite( MIXING_PUMP_RELAY_PIN, mixingPumpRelayState );
	// Extractor
	extractorRelayState = EEPROM.read( extractorFlashAddress );
	digitalWrite( EXTRACTOR_RELAY_PIN, extractorRelayState );		
	// Lights
	lightsRelayState = EEPROM.read( lightsFlashAddress );
	digitalWrite( LIGHTS_RELAY_PIN, lightsRelayState );
	// Feeding pump
	feedingPumpRelayState = EEPROM.read( feedingPumpFlashAddress );
	digitalWrite( FEEDING_PUMP_RELAY_PIN, feedingPumpRelayState );
	// Fan
	fanRelayState = EEPROM.read( fanFlashAddress );
	digitalWrite( FAN_RELAY_PIN, fanRelayState );
	// Airco
	aircoRelayState = EEPROM.read( aircoFlashAddress );
	digitalWrite( AIRCO_RELAY_PIN, aircoRelayState );
	// Lcd
	Wire.begin(); 
	lcd.begin();
	lcd.clear();
	lcd.setCursor (0, 0); 
	lcd.print("Starting"); 
	lcd.setCursor (0, 2); 
	lcd.print("roomID: " + roomID); 
	updateInterval = EEPROM.readFloat(updateIntervalFlashAddress);
	if (!updateInterval || updateInterval == 4294967295)
	{
		updateInterval = 10000;
	}
	backlightOn = EEPROM.read(backlightOnFlashAddress);
	if (backlightOn)
	{
		lcd.backlight();
	}
	else
	{
		lcd.noBacklight();
	}
	sendSerialData("c", "restart", "1");
}

void loop() 
{
	solValveBtnState = changeRelayStateManually( solValveBtnState, solValveRelayState, SOLENOID_VALVE_BTN_PIN, SOLENOID_VALVE_RELAY_PIN, solValveFlashAddress, "m/" + roomID + "/sv-btn" );
	drainPumpBtnState = changeRelayStateManually( drainPumpBtnState, drainPumpRelayState, DRAIN_PUMP_BTN_PIN, DRAIN_PUMP_RELAY_PIN, drainPumpFlashAddress,"m/" + roomID + "/dp-btn" );
	mixingPumpBtnState = changeRelayStateManually( mixingPumpBtnState, mixingPumpRelayState, MIXING_PUMP_BTN_PIN, MIXING_PUMP_RELAY_PIN, mixingPumpFlashAddress, "m/" + roomID + "/mp-btn" );
	extractorBtnState = changeRelayStateManually( extractorBtnState, extractorRelayState, EXTRACTOR_BTN_PIN, EXTRACTOR_RELAY_PIN, extractorFlashAddress, "m/" + roomID + "/ex-btn" );
	lightsBtnState = changeRelayStateManually( lightsBtnState, lightsRelayState, LIGHTS_BTN_PIN, LIGHTS_RELAY_PIN, lightsFlashAddress, "m/" + roomID + "/li-btn" );
	feedingPumpBtnState = changeRelayStateManually( feedingPumpBtnState, feedingPumpRelayState, FEEDING_PUMP_BTN_PIN, FEEDING_PUMP_RELAY_PIN, feedingPumpFlashAddress, "m/" + roomID + "/fp-btn" );
	fanBtnState = changeRelayStateManually( fanBtnState, fanRelayState, FAN_BTN_PIN, FAN_RELAY_PIN, fanFlashAddress, "m/" + roomID + "/fa-btn" );
	aircoBtnState = changeRelayStateManually( aircoBtnState, aircoRelayState, AIRCO_BTN_PIN, AIRCO_RELAY_PIN, aircoFlashAddress, "m/" + roomID + "/ai-btn" );
	lcdBacklightBtn();
	readSerialData( );
	updateDisplayValues();
}