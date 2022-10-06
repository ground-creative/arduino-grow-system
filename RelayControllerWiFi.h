#ifndef RELAY_CONTROLLER_WIFI_CONFIG
	#include "RelayControllerWiFiDefaultConfig.h"
#endif

// Net tools
#include <NetTools.h>

// Json library
#include <ArduinoJson.h>

// Software serial
#include "SoftwareSerial.h"

// Variables
const String mqttClientID = roomID + "-display";
const String mqttTopic = roomID + "/display";
String content = "";
bool mqttConnected = false;

// Init software serial
SoftwareSerial nodemcu(RXD2, TXD2); //RX, TX

void sendSerialData(String dataType, String message, String payload)
{
	DynamicJsonDocument doc(256);
	doc["t"] = dataType;
	doc["m"] = message;
	doc["p"] = payload;
	serializeJson(doc, Serial);
	Serial.println("");
	serializeJson(doc, nodemcu);
	delay(1000);
}

void mqtt_callback( char* topic, byte* payload, unsigned int length ) 
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
		Serial.println("Sending serial command");
		sendSerialData("command", topic, String((char)payload[0]));
		delay(300);
	}
	else
	{
		//Serial.println("Uncaught mqtt callback");
		Serial.println("Sending serial json data");
		StaticJsonDocument<256> doc;
		deserializeJson(doc, (const byte*)payload, length);
		char buffer[256];
		serializeJson(doc, buffer);
		sendSerialData("json", topic, String(buffer));
		delay(300);
		if(String( topic ) == roomID + "/relay-controller-restart")
		{
			delay(1000);
			ESP.restart();
		}
	}
}

// Wifi tools
NetTools::WIFI network(ssid, password);

// Mqtt tools
NetTools::MQTT mqtt(mqttClientID, mqtt_server, mqtt_callback);

void mqttSubscribe(String roomID)
{
	Serial.println("Subscribing to mqtt messages");
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
	mqtt.subscribe(const_cast<char*>(String(roomID + "/air-values").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/relay-controller-display-update-interval").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/relay-controller-display-backlight").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/relay-controller-restart").c_str()));
}

String getData(String data, char delimiter, int sequence)
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
					if(String(msgType) == "info")
					{
						Serial.println("Received serial info data");
					}
					else if (String(msgType) == "command")
					{
						Serial.println("Received serial command");
					}
					else if (String(msgType) == "mqtt")
					{
						if (mqttConnected != true)
						{
							Serial.println("Received serial mqtt data but we are not connected to the server");
						}
						else
						{
							Serial.println("Received serial mqtt data");
							mqtt.publish(const_cast<char*>(message), const_cast<char*>(payload));
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
	//if (!START_SERIAL_DEBUG)
	//{
		//pinMode(WIFI_LED_PIN, OUTPUT);
		//pinMode(MQTT_LED_PIN, OUTPUT);
		//digitalWrite(WIFI_LED_PIN, LOW);
		//digitalWrite(MQTT_LED_PIN, LOW);
	
	//}
	//else
	//{
		// Serial
		//Serial.begin(9600, SERIAL_8N1, SERIAL_TX_ONLY);
		//Serial.begin(9600);
		delay(1000);
		//while ( !Serial ){ ; }         // wait for serial port to connect. Needed for native USB port only
	//}
	// Serial2
	nodemcu.begin(BAUD_RATE);
	Serial.println("Serial started ");
	sendSerialData("info","Connecting to WiFi","");
	// Connect to Wi-Fi
	network.connect();
	//if (!START_SERIAL_DEBUG)
	//{
		//digitalWrite(WIFI_LED_PIN, HIGH);
	//}
	sendSerialData("info", "Connected to WiFi", network.localAddress().toString());
	delay(2000);
	sendSerialData("info", "Connecting to mqtt", "");
	if (mqtt.connect(mqtt_username, mqtt_password))
	{
		sendSerialData("info", "Connected to mqtt", "");
		// Subscribe to mqtt messages
		mqttSubscribe(roomID);
		sendSerialData("info", "Subscribed to mqtt", "");
		//if (!START_SERIAL_DEBUG)
		//{
			//digitalWrite(MQTT_LED_PIN, HIGH);
		//}
		mqttConnected = true;
	}
}

void loop() 
{
	// check if we are connected to WiFi
	if (network.status( ) != WL_CONNECTED)
	{
		network.check();
	}
	if (!mqtt.isConnected())
	{
		mqttConnected = false;
		//if (!START_SERIAL_DEBUG)
		//{
			//digitalWrite(MQTT_LED_PIN, LOW);
		//}
		sendSerialData("info", "Connecting to mqtt", "");
		// Reconnected to mqtt server
		if(mqtt.connect(mqtt_username, mqtt_password))
		{
			sendSerialData("info", "Connected to mqtt", "");
			// Subscribe to mqtt messages
			mqttSubscribe(roomID);
			sendSerialData("info", "Subscribed to mqtt", "");
			//if (!START_SERIAL_DEBUG)
			//{
				//digitalWrite(MQTT_LED_PIN, HIGH);
			//}
			mqttConnected = true;
		}
	}
	// Loop mqtt client
	mqtt.loop();
	// Read incoming msgs from Serial2
	readSerialData( );
}