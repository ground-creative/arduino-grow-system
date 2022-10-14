/**
	Grow system air sensors component
	Author: Carlo Pietrobattista
	Version: 1.0
*/

#ifndef AIR_SENSORS_CONFIG
	#include "airSensorsDefaultConfig.h"
#endif
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "SoftwareSerial.h"
#include <EEPROM.h>
#include <MQUnifiedsensor.h>

DHT dht(DHT_PIN, DHT_TYPE);
OneWire oneWire(DS1820_PIN);
DallasTemperature sensors(&oneWire);
MQUnifiedsensor MQ135(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ135_PIN, MQ_TYPE);
SoftwareSerial nodemcu(RXD2, TXD2);
SSD1306AsciiAvrI2c oled;

float outTemp = 0.0, inTemp = 0.0, inHum = 0.0, inCo2 = 0, RO = 0;
String componentID = "air-sensors";
const String wifiIP = "";
unsigned long previousMillis = 0; 
unsigned int outTempSensorCountRetries = 0;
bool oledOn;

void softwareReset(unsigned long delayMillis)
{
	uint32_t resetTime = millis() + delayMillis;
	while (resetTime > millis()) { }
	asm volatile ( "jmp 0");  
}

void calibrateMQ135Sensor()
{
	/*****************************  MQ CAlibration ********************************************/ 
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

void sendSerialData(String dataType, String message, String payload)
{
	StaticJsonDocument<100> doc;
	doc["t"] = dataType;
	doc["m"] = message;
	doc["p"] = payload;
	Serial.println("Sending serial data: ");
	serializeJson(doc, Serial);
	serializeJson(doc, nodemcu);
	nodemcu.print('\n');
	Serial.println();
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
		StaticJsonDocument<100> doc;
		deserializeJson(doc, content);
		const char* msgType = doc["t"];
		const char* message = doc["m"];
		const char* payload = doc["p"];
		Serial.println("");
		if(String(msgType) == "i")
		{
			Serial.println("Received serial info data");
			if (String(message) == "Connected to WiFi")
			{
				wifiIP = payload;
			}
			oled.clear();
			oled.setRow(1);
			oled.setCursor(0, 0);
			oled.set1X();
			oled.print(message);
		}
		else if (String(msgType) == "c")
		{
			Serial.println("Received serial command");
			if (String(message) == "restart")
			{
				softwareReset(60);
			}
			else if (String(message) == "display-backlight")
			{
				if (String(payload) == "1")
				{
					oledOn = 1;
				}
				else
				{
					oledOn = 0;
					oled.clear();
				}
				EEPROM.write(oledFlashAddress, oledOn);
			}
			else if (String( message ) == "config-component-id")
			{
				Serial.println("Sending config data");
				oled.clear();
				oled.setRow(1);
				oled.setCursor(0, 0);
				oled.set1X();
				oled.print("Sending config data to wifi chip");
				sendSerialData("j", "c", "{\"componentID\":\"air-sensors\"}");
			}
			else if (String( message ) == "calibrate-mq135")
			{
				calibrateMQ135Sensor();
				delay(1000);
				softwareReset(60); 
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

void updateDHTValues()
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

void updateCo2Values()
{
	if (!USE_MQ135){ return; }
	MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
	MQ135.setA(110.47); MQ135.setB(-2.862); // Configure the equation to calculate CO2 concentration value
	float CO2 = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
	inCo2 = CO2 + co2BaseValue;
	Serial.print("MQ135 CO2: ");
	Serial.print(inCo2);
	Serial.println();
}

// NOT IN USE
void calculateCo2Values()
{
	if (!USE_MQ135){ return; }
	int co2now[10];                               //int array for co2 readings
	int co2raw = 0;                               //int for raw value of co2
	int co2comp = 0;                              //int for compensated co2 
	int co2ppm = 0;                               //int for calculated ppm
	int zzz = 0;                                  //int for averaging
	int grafX = 0;                                //int for x value of graph
	for (int x = 0; x<10; x++)                    //samplpe co2 10x over 2 seconds
	{                  
		co2now[x] = analogRead(MQ135_PIN);
		delay(200);
	}
	for(int x = 0;x<10;x++)                      //add samples together
	{                     
		zzz=zzz + co2now[x];
	}
	co2raw = zzz/10;                              //divide samples by 10
	co2comp = co2raw - co2Zero;                   //get compensated value
	co2ppm = map(co2comp,0,1023,400,5000);        //map value for atmospheric levels
	Serial.print("MQ135 CO2: ");
	Serial.print(co2ppm);
	Serial.println();
	inCo2 = co2ppm;
}

void updateDS1820Values()
{
	if (!USE_DS1820){ return; }
	sensors.requestTemperatures(); 
	float outTempMem = sensors.getTempCByIndex(0);
	if (outTempMem > -127.00)
	{
		outTemp = outTempMem;
		outTempSensorCountRetries = 0;
	}
	else if (outTempSensorCountRetries == outTempSensorMaxRetries)
	{
		Serial.println("Cannot read temperature DS1820 sensor problem, resetting");
		//delay( 1000 );
		softwareReset(60);  // call reset
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

void writeConfig(int address, String word) 
{
	delay(10);
	for (int i = 0; i < word.length(); ++i) 
	{
		//Serial.println(word[i]);
		EEPROM.write(address + i, word[i]);
	}
	EEPROM.write(address + word.length(), '\0');
}

String readConfig(int addr)
{
	String result = "";
	uint8_t count = 0;
	while (count < 21) 
	{
		//yield();
		char byte = EEPROM.read(addr + count);
		if (byte == '\0') 
		{
			break;
		}
		else 
		{
			result.concat(byte);
		}
		count++;
	}
	result = result.substring(0, (result.length()));
	return result;
}

void setup() 
{
	Serial.begin(9600);
	nodemcu.begin(BAUD_RATE);
	delay(1000);
	Serial.println("Component started with config " + roomID +  ":" + componentID);
	sendSerialData("c", "restart", "1");
	#if RST_PIN >= 0
		oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
	#else 
		oled.begin(&Adafruit128x64, I2C_ADDRESS);
	#endif
	oled.setFont(Adafruit5x7);
	oled.clear();
	oledOn = EEPROM.read( oledFlashAddress );
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
	//pinMode(MQ135_PIN,INPUT); 
	if (USE_DS1820){ sensors.begin( ); }
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
}

void loop() 
{
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis >= updateInterval) 
	{
		previousMillis = currentMillis;
		updateDS1820Values();
		updateDHTValues();
		updateCo2Values();
		updateDisplayValues();
		String data = "{\"o\":\"" + String(outTemp, 1) + "\",\"i\":\"" + String(inTemp, 1) + "\",\"h\":\"" + String(inHum, 1) + "\",\"c\":\"" + String(inCo2, 0) + "\"}";
		sendSerialData("m", "air" , data);
	}
	readSerialData( );
}