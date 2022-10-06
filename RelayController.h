#ifndef _RELAY_CONTROLLER_DEFAULT_CONFIG_
	#define _RELAY_CONTROLLER_DEFAULT_CONFIG_	
	#include "RelayControllerDefaultConfig.h"
#endif

// Net tools library
#include <NetTools.h>

// Flash write relay state
#include <EEPROM.h>

// Lcd screen library
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// Arduino library
#include "Arduino.h"

// Io expander library
#include "PCF8575.h"

// Json library
#include <ArduinoJson.h>

// Software serial
#include "SoftwareSerial.h"

// Set io expander address
PCF8575 pcf8575(0x20);

// Set lcd address
LiquidCrystal_I2C lcd(lcdAddress, lcdColumns, lcdRows);

// Set sotfware serial pins
SoftwareSerial nodemcu(RXD2, TXD2); //RX, TX

// Variables
float water_temp = 0.0, ph = 0.0, ec = 0.0, ppm = 0;
int water_level = 0, solValveBtnState, drainPumpBtnState, mixingPumpBtnState, extractorBtnState, lightsBtnState, feedingPumpBtnState, fanBtnState, aircoBtnState;
float t = 0.0, h = 0.0, dtemperature = 0.0;
unsigned long previousMillis = 0;

unsigned long button_time = 0;  
unsigned long last_button_time = 0;

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
  doc["type"] = dataType;
  doc["message"] = message;
  doc["payload"] = payload;
  Serial.println("Sending serial data: ");
  serializeJson(doc, Serial);
  serializeJson(doc, nodemcu);
  delay(1000);
}

void changeRelayState(int value, int relayPin, int ledPin, int flashAddress)
{
	//if (value == 49) // switch on
	if(value)
	{
		Serial.println("Switching on relay");
		pcf8575.digitalWrite(relayPin, LOW);
		digitalWrite(ledPin, HIGH);
		EEPROM.write(flashAddress, 0);
	} 
	//else  if(value == 48) // switch off 
	else
	{
		Serial.println("Switching off relay");
		pcf8575.digitalWrite(relayPin, HIGH);
		digitalWrite(ledPin, LOW);
		EEPROM.write(flashAddress, 1);
	}    
	EEPROM.commit();
}

int changeRelayStateManually(int curBtnState, int &relayState, int btnPin, int relayPin, int ledPin, int flashAddress, String mqttTopic = "")
{
	int lastButtonState = curBtnState;
	curBtnState = debounceButton(relayState, btnPin);
	if(lastButtonState == HIGH && curBtnState == LOW) 
	{
		Serial.print("The button is pressed ");
		Serial.println(mqttTopic);
		relayState = !relayState;
		pcf8575.digitalWrite(relayPin, relayState);
		digitalWrite(ledPin, ("LOW" == (relayState ? "HIGH" : "LOW")));
		EEPROM.write(flashAddress, relayState);
		EEPROM.commit();
		sendSerialData("mqtt", mqttTopic, String("LOW" == (relayState ? "HIGH" : "LOW")));
	}
	return curBtnState;
}

void processSerialCommand(String message, String payload)
{
	if(message == roomID + "/water-valve")
	{
		changeRelayState(payload.toInt(), SOLENOID_VALVE_RELAY_PIN, SOLENOID_VALVE_LED_PIN, SOLENOID_VALVE_BTN_PIN);
	}
	if(message == roomID + "/drain-pump")
	{
		changeRelayState(payload.toInt(), DRAIN_PUMP_RELAY_PIN, DRAIN_PUMP_LED_PIN, DRAIN_PUMP_BTN_PIN);
	}
	if(message == roomID + "/mixing-pump")
	{
		changeRelayState(payload.toInt(), MIXING_PUMP_RELAY_PIN, MIXING_PUMP_LED_PIN, MIXING_PUMP_BTN_PIN);
	}
	if(message == roomID + "/extractor")
	{
		changeRelayState(payload.toInt(), EXTRACTOR_RELAY_PIN, EXTRACTOR_LED_PIN, EXTRACTOR_BTN_PIN);
	}
	if(message == roomID + "/lights")
	{
		changeRelayState(payload.toInt(), LIGHTS_RELAY_PIN, LIGHTS_LED_PIN, LIGHTS_BTN_PIN);
	}
	if(message == roomID + "/feeding-pump")
	{
		changeRelayState(payload.toInt(), FEEDING_PUMP_RELAY_PIN, FEEDING_PUMP_LED_PIN, FEEDING_PUMP_BTN_PIN);
	}
	if(message == roomID + "/fan")
	{
		changeRelayState(payload.toInt(), FAN_RELAY_PIN, FAN_LED_PIN, FAN_BTN_PIN);
	}
	if(message == roomID + "/airco")
	{
		changeRelayState(payload.toInt(), AIRCO_RELAY_PIN, AIRCO_LED_PIN, AIRCO_BTN_PIN);
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
		const char* msgType = doc["type"];
		const char* message = doc["message"];
		const char* payload = doc["payload"];
		if(String(msgType) == "info")
		{
			Serial.println("Received serial info data");
			lcd.clear( );
			lcd.setCursor ( 0, 0 ); 
			lcd.print( message ); 
			lcd.setCursor ( 0, 1 ); 
			lcd.print( payload );
		}
		else if(String(msgType) == "command")
		{
			Serial.println("Received serial command");
			//Serial.println(msgType);
			//Serial.println(message);
			//Serial.println(payload);
			processSerialCommand(message, payload);
		}
		else
		{
			Serial.println("Received serial uncaught data");
		}
		Serial.println(content);
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
	// Set pin mode solenoid valve
	pcf8575.pinMode( SOLENOID_VALVE_RELAY_PIN, OUTPUT );
	pinMode( SOLENOID_VALVE_BTN_PIN, INPUT_PULLUP );
	pinMode( SOLENOID_VALVE_LED_PIN, OUTPUT );
	// Set pin mode drain pump
	pcf8575.pinMode( DRAIN_PUMP_RELAY_PIN, OUTPUT );
	pinMode( DRAIN_PUMP_BTN_PIN, INPUT_PULLUP );
	pinMode( DRAIN_PUMP_LED_PIN, OUTPUT );
	// Set pin mode mixing pump
	pcf8575.pinMode( MIXING_PUMP_RELAY_PIN, OUTPUT );
	pinMode( MIXING_PUMP_BTN_PIN, INPUT_PULLUP );
	pinMode( MIXING_PUMP_LED_PIN, OUTPUT );
	// Set pin mode extractor
	pcf8575.pinMode( EXTRACTOR_RELAY_PIN, OUTPUT );
	pinMode( EXTRACTOR_BTN_PIN, INPUT_PULLUP );
	pinMode( EXTRACTOR_LED_PIN, OUTPUT );
	// Set pin mode lights
	pcf8575.pinMode( LIGHTS_RELAY_PIN, OUTPUT );
	pinMode( LIGHTS_BTN_PIN, INPUT_PULLUP );
	pinMode( LIGHTS_LED_PIN, OUTPUT );
	// Set pin mode feeding pump
	pcf8575.pinMode( FEEDING_PUMP_RELAY_PIN, OUTPUT );
	pinMode( FEEDING_PUMP_BTN_PIN, INPUT_PULLUP );
	pinMode( FEEDING_PUMP_LED_PIN, OUTPUT );
	// Set pin mode fan
	pcf8575.pinMode( FAN_RELAY_PIN, OUTPUT );
	pinMode( FAN_BTN_PIN, INPUT_PULLUP );
	pinMode( FAN_LED_PIN, OUTPUT );
	// Set pin mode airco
	pcf8575.pinMode( AIRCO_RELAY_PIN, OUTPUT );
	pinMode( AIRCO_BTN_PIN, INPUT_PULLUP );
	pinMode( AIRCO_LED_PIN, OUTPUT );
	// IO Extender
	pcf8575.begin();
	// EEPROM
	EEPROM.begin( 8 );
	// Solenoid valve
	solValveRelayState = EEPROM.read( solValveFlashAddress );
	pcf8575.digitalWrite( SOLENOID_VALVE_RELAY_PIN, solValveRelayState );
	digitalWrite( SOLENOID_VALVE_LED_PIN, ( "LOW" == ( solValveRelayState ? "HIGH" : "LOW" ) ) );
	// Drain pump
	drainPumpRelayState = EEPROM.read( drainPumpFlashAddress );
	pcf8575.digitalWrite( DRAIN_PUMP_RELAY_PIN, drainPumpRelayState );
	digitalWrite( DRAIN_PUMP_LED_PIN, ( "LOW" == ( drainPumpRelayState ? "HIGH" : "LOW" ) ) );
	// Mixing pump
	mixingPumpRelayState = EEPROM.read( mixingPumpFlashAddress );
	pcf8575.digitalWrite( MIXING_PUMP_RELAY_PIN, mixingPumpRelayState );
	digitalWrite( MIXING_PUMP_LED_PIN, ( "LOW" == ( mixingPumpRelayState ? "HIGH" : "LOW" ) ) );
	// Extractor
	extractorRelayState = EEPROM.read( extractorFlashAddress );
	pcf8575.digitalWrite( EXTRACTOR_RELAY_PIN, extractorRelayState );
	digitalWrite( EXTRACTOR_LED_PIN, ( "LOW" == ( extractorRelayState ? "HIGH" : "LOW" ) ) );
	// Lights
	lightsRelayState = EEPROM.read( lightsFlashAddress );
	pcf8575.digitalWrite( LIGHTS_RELAY_PIN, lightsRelayState );
	digitalWrite( LIGHTS_LED_PIN, ( "LOW" == ( lightsRelayState ? "HIGH" : "LOW" ) ) );
	// Feeding pump
	feedingPumpRelayState = EEPROM.read( feedingPumpFlashAddress );
	pcf8575.digitalWrite( FEEDING_PUMP_RELAY_PIN, feedingPumpRelayState );
	digitalWrite( FEEDING_PUMP_LED_PIN, ( "LOW" == ( feedingPumpRelayState ? "HIGH" : "LOW" ) ) );
	// Fan
	fanRelayState = EEPROM.read( fanFlashAddress );
	pcf8575.digitalWrite( FAN_RELAY_PIN, fanRelayState );
	digitalWrite( FAN_LED_PIN, ( "LOW" == ( fanRelayState ? "HIGH" : "LOW" ) ) );
	// Airco
	aircoRelayState = EEPROM.read( aircoFlashAddress );
	pcf8575.digitalWrite( AIRCO_RELAY_PIN, aircoRelayState );
	digitalWrite( AIRCO_LED_PIN, ( "LOW" == ( aircoRelayState ? "HIGH" : "LOW" ) ) );

	//attachInterrupt( digitalPinToInterrupt(SOLENOID_VALVE_BTN_PIN), waterValveISR, CHANGE);

	// Lcd
	Wire.begin( ); 
	lcd.begin();
	lcd.backlight( );
	lcd.clear( );
	lcd.setCursor ( 0, 0 ); 
	lcd.print( "Starting" ); 
	
	delay( 300 );
}

void loop() 
{
	solValveBtnState = changeRelayStateManually( solValveBtnState, solValveRelayState, SOLENOID_VALVE_BTN_PIN, SOLENOID_VALVE_RELAY_PIN, SOLENOID_VALVE_LED_PIN, solValveFlashAddress, "device-manual-action/" + roomID + "/solenoid-valve-btn-pressed" );
	drainPumpBtnState = changeRelayStateManually( drainPumpBtnState, drainPumpRelayState, DRAIN_PUMP_BTN_PIN, DRAIN_PUMP_RELAY_PIN, DRAIN_PUMP_LED_PIN, drainPumpFlashAddress,"device-manual-action/" + roomID + "/drain-pump-btn-pressed" );
	mixingPumpBtnState = changeRelayStateManually( mixingPumpBtnState, mixingPumpRelayState, MIXING_PUMP_BTN_PIN, MIXING_PUMP_RELAY_PIN, MIXING_PUMP_LED_PIN, mixingPumpFlashAddress, "device-manual-action/" + roomID + "/mixing-pump-btn-pressed" );
	extractorBtnState = changeRelayStateManually( extractorBtnState, extractorRelayState, EXTRACTOR_BTN_PIN, EXTRACTOR_RELAY_PIN, EXTRACTOR_LED_PIN, extractorFlashAddress, "device-manual-action/" + roomID + "/extractor-btn-pressed" );
	lightsBtnState = changeRelayStateManually( lightsBtnState, lightsRelayState, LIGHTS_BTN_PIN, LIGHTS_RELAY_PIN, LIGHTS_LED_PIN, lightsFlashAddress, "device-manual-action/" + roomID + "/lights-btn-pressed" );
	feedingPumpBtnState = changeRelayStateManually( feedingPumpBtnState, feedingPumpRelayState, FEEDING_PUMP_BTN_PIN, FEEDING_PUMP_RELAY_PIN, FEEDING_PUMP_LED_PIN, feedingPumpFlashAddress, "device-manual-action/" + roomID + "/feeding-pump-btn-pressed" );
	fanBtnState = changeRelayStateManually( fanBtnState, fanRelayState, FAN_BTN_PIN, FAN_RELAY_PIN, FAN_LED_PIN, fanFlashAddress, "device-manual-action/" + roomID + "/fan-btn-pressed" );
	aircoBtnState = changeRelayStateManually( aircoBtnState, aircoRelayState, AIRCO_BTN_PIN, AIRCO_RELAY_PIN, AIRCO_LED_PIN, aircoFlashAddress, "device-manual-action/" + roomID + "/airco-btn-pressed" );
	readSerialData( );
	unsigned long currentMillis = millis();
	if ( currentMillis - previousMillis >= updateInterval ) 
	{
		// save the last time you updated the values
		previousMillis = currentMillis;

		lcd.clear( );
		lcd.setCursor( 0, 0 );
		lcd.print( String( dtemperature, 1 ).c_str( ) );
		lcd.print( ( char ) 223 );

		lcd.print( " | " );

		lcd.print( String( t, 1 ).c_str( ) );
		lcd.print( ( char ) 223 );
		lcd.print( "  " );
		lcd.print( String( h, 1 ).c_str());
		lcd.print("%");

		lcd.setCursor( 7, 1 );
		lcd.print( "Water" );

		lcd.setCursor( 0, 2 );
		lcd.print( "ph:" );
		lcd.print( String( ph, 2 ).c_str() );

		lcd.setCursor( 15, 2 );
		lcd.print( String( water_temp, 1 ).c_str( ) );
		lcd.print( ( char ) 223 );

		lcd.setCursor( 8, 3 );
		lcd.print( water_level );
		lcd.print( "%" );

		lcd.setCursor( 0, 3 );
		lcd.print( "ec:" );
		lcd.print( String( ec, 1 ).c_str( ) );

		lcd.setCursor( 14, 3 );
		lcd.print( String( ppm, 0 ).c_str( ) );
		lcd.print( "ppm" );
	}
	//delay(10000);
}