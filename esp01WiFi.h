#ifndef WIFI_CONFIG
	#include "esp01WiFiDefaultConfig.h"
#endif
#include <NetTools.h>
#include <ArduinoJson.h>
#include "SoftwareSerial.h"
#include <EEPROM.h>

String content , componentID, mqttClientID;
bool mqttConnected = false;

String defaultComponentID = "wifi-chip";

SoftwareSerial nodemcu(RXD2, TXD2);
NetTools::WIFI network(ssid, password);

void sendSerialData(const String& dataType, const String& message, const String& payload)
{
	DynamicJsonDocument doc(256);
	doc["t"] = dataType;
	doc["m"] = message;
	doc["p"] = payload;
	Serial.println();
	Serial.println("Sending serial data");
	serializeJson(doc, Serial);
	Serial.println("");
	serializeJson(doc, nodemcu);
	delay(1000);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{  
	Serial.println("");
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) 
	{
		Serial.print((char)payload[i]);
	}
	if((char)payload[0] == '1' || (char)payload[0] == '0')
	{
		Serial.println("");
		Serial.println("Sending serial command");
		if ( String(topic) == roomID + "/air-sensors-restart" || 
				String(topic) == roomID + "/main-controller-restart" )
		{
			topic = "restart";
		}
		else if ( String(topic) == roomID + "/air-sensors-display-backlight"  || 
				String(topic) == roomID + "/main-controller-display-backlight" )
		{
			topic = "display-backlight";
		}
		else if ( String(topic) == roomID + "/config-component-id")
		{
			topic = "config-component-id";
		}
		else if ( String(topic) == roomID + "/" + componentID + "/calibrate-mq135")
		{
			topic = "calibrate-mq135";
		}
		sendSerialData("c", topic, String((char)payload[0]));
		delay(300);
	}
	else
	{
		Serial.println("");
		Serial.println("Sending serial json data");
		StaticJsonDocument<256> doc;
		deserializeJson(doc, (const byte*)payload, length);
		char buffer[256];
		serializeJson(doc, buffer);
		if ( String(topic) == roomID + "/air-sensors" )
		{
			Serial.println(buffer);
			topic = "air";
		}
		if ( String(topic) == roomID + "/water-level" )
		{
			Serial.println(buffer);
			topic = "wl";
		}
		sendSerialData("j", topic, String(buffer));
		delay(300);
	}
}

NetTools::MQTT mqtt(mqtt_server, mqtt_callback);

void mqttSubscribe(const String& roomID)
{
	Serial.println("Subscribing to mqtt messages");
	mqtt.subscribe(const_cast<char*>(String(roomID + "/config-component-id").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-backlight").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-restart").c_str()));
	if (componentID == "air-sensors")
	{
		//mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-backlight").c_str()));
		//mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-restart").c_str()));
		mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "/calibrate-mq135").c_str()));
	}
	else if (componentID == "main-controller")
	{
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
		//mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-backlight").c_str()));
		//mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-restart").c_str()));
	}
}

String getData(const String& data, char delimiter, int sequence)
{
	int stringData = 0;
	String sectionData = "";
	for (int i = 0; i < data.length() - 1; i++)
	{
		if (data[i] == delimiter)
		{
			stringData++;
		}
		else if (stringData == sequence)
		{
			sectionData.concat(data[i]);
		}
		else if (stringData > sequence)
		{
			return sectionData;
			break;
		}
	}
	return sectionData;
}

String readConfig(int addr)
{
	String result = "";
	uint8_t count = 0;
	while (count < 31) 
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

void writeConfig(int address, String word) 
{
	delay(10);
	for (int i = 0; i < word.length(); ++i) 
	{
		//Serial.println(word[i]);
		EEPROM.write(address + i, word[i]);
	}
	EEPROM.write(address + word.length(), '\0');
	EEPROM.commit();
}

void readSerialData( )
{
	if (nodemcu.available() > 0) 
	{
		while (nodemcu.available() > 0) 
		{
			content = nodemcu.readString( );
			String ab[content.length()];
			for (int a = 0; a < content.length() - 1; a++)
			{
				ab[a] = getData(content, '\n', a);
				if (ab[a] != NULL)
				{
					DynamicJsonDocument doc(2048);
					deserializeJson(doc, ab[a]);
					const char* msgType = doc["t"];
					const char* message = doc["m"];
					const char* payload = doc["p"];
					Serial.println("");
					if(String(msgType) == "i")
					{
						Serial.println("Received serial info data");
					}
					else if (String(msgType) == "c")
					{
						Serial.println("Received serial command");
						if(String(message) == "restart")
						{
							ESP.restart();
						}
					}
					else if (String(msgType) == "m")
					{
						if (mqttConnected != true)
						{
							Serial.println("Received serial mqtt data but we are not connected to the server");
						}
						else
						{
							Serial.println("Received serial mqtt data");
							if ( String(message) == "air" )
							{
								String go = roomID + "/" + componentID;
								char charBuf[go.length()+1];
								go.toCharArray(charBuf, go.length()+1);
								Serial.println(charBuf);
								mqtt.publish(charBuf, const_cast<char*>(payload));
							}
							else
							{
								mqtt.publish(const_cast<char*>(message), const_cast<char*>(payload));
							}
						}
					}
					else if (String(msgType) == "j")
					{
						Serial.println("Received serial json data");
						StaticJsonDocument<200> doc1;
						deserializeJson(doc1, doc["p"]);
						if (String(message) == "c")
						{        
							componentID = String( doc1["componentID"] );
							writeConfig(componentIDFlashAddress, componentID);
							delay(1000);
							ESP.restart();
						}
					}
					else
					{
						Serial.println("Received serial uncaught data");
					}
					Serial.println(ab[a]);
				}
			}
		} 
	}
}

void setup()  
{
	#ifdef DEBUG_SERIAL
		Serial.begin(9600);
	#else
		pinMode(WIFI_LED_PIN, OUTPUT);
		digitalWrite(WIFI_LED_PIN, LOW);
		pinMode(MQTT_LED_PIN, OUTPUT);
		digitalWrite(MQTT_LED_PIN, LOW);
	#endif
	nodemcu.begin(BAUD_RATE);
	delay(1000);
	Serial.println("Starting...");
	EEPROM.begin(512);
	componentID = readConfig(componentIDFlashAddress);
	if (!componentID || componentID.length() == 0 || componentID.length() == 31)
	{
		Serial.println( "Please call service config-component-id" );
		sendSerialData("i", "Please call service config-component-id", "");
		componentID = defaultComponentID;
		delay(5000);
	}
	else
	{
		sendSerialData("i", "cid:" + componentID, "");
		delay(2000);
	}
	mqttClientID = roomID + "-" + componentID;
	Serial.println("Wifi chip started with config " + roomID +  ":" + componentID);
	sendSerialData("i", "Connecting to WiFi", "");
	network.connect();
	#ifndef DEBUG_SERIAL
		digitalWrite(WIFI_LED_PIN, HIGH);
	#endif
	sendSerialData("i", "Connected to WiFi", network.localAddress().toString());
	delay(2000);
	sendSerialData("i", "Connecting to mqtt", "");
	if ( mqtt.connect(mqttClientID, mqtt_username, mqtt_password) )
	{
		sendSerialData("i", "Connected to mqtt", "");
		mqttSubscribe(roomID);
		sendSerialData("i", "Subscribed to mqtt", "");
		#ifndef DEBUG_SERIAL
			digitalWrite(MQTT_LED_PIN, HIGH);
		#endif
		mqttConnected = true;
	}
}

void loop() 
{
	if (network.status( ) != WL_CONNECTED)
	{
		network.check();
	}
	if (!mqtt.isConnected())
	{
		mqttConnected = false;
		#ifndef DEBUG_SERIAL
			digitalWrite(MQTT_LED_PIN, LOW);
		#endif
		sendSerialData("i", "Connecting to mqtt", "");
		if(mqtt.connect(mqttClientID, mqtt_username, mqtt_password))
		{
			sendSerialData("i", "Connected to mqtt", "");
			mqttSubscribe(roomID);
			sendSerialData("i", "Subscribed to mqtt", "");
			#ifndef DEBUG_SERIAL
				digitalWrite(MQTT_LED_PIN, HIGH);
			#endif
			mqttConnected = true;
		}
	}
	mqtt.loop();
	readSerialData( );
}