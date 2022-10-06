#ifndef _RELAY_CONTROLLER_WIFI_DEFAULT_CONFIG_
	#define _RELAY_CONTROLLER_WIFI_DEFAULT_CONFIG_	
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

SoftwareSerial nodemcu(RXD2, TXD2); //RX, TX

void sendSerialData(String dataType, String message, String payload)
{
	DynamicJsonDocument doc(2048);
	doc["type"] = dataType;
	doc["message"] = message;
	doc["payload"] = payload;
	Serial.println("Sending serial data: ");
	serializeJson(doc, Serial);
	serializeJson(doc, nodemcu);
	Serial.println("");
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
		sendSerialData("command", topic, String((char)payload[0]));
	}
	//sendSerialData("command", topic, String((char *)payload));
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
	mqtt.subscribe(const_cast<char*>(String(roomID + "/display-update-interval").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/display-backlight").c_str()));
	mqtt.subscribe(const_cast<char*>(String(roomID + "/display-restart").c_str()));
}

void readSerialData( )
{
	if(nodemcu.available() > 0) 
	{
		while (nodemcu.available() > 0) 
		{
			content = nodemcu.readString( );
			Serial.println("Received serial data");
			Serial.println( content );
			DynamicJsonDocument doc(2048);
			deserializeJson(doc, content);
			const char* msgType = doc["type"];
			const char* message = doc["message"];
			const char* payload = doc["payload"];
			if(String(msgType) == "info")
			{
				Serial.println("Received serial info data");
			}
			else if(String(msgType) == "command")
			{
				Serial.println("Received serial command");
				//Serial.println(msgType);
				//Serial.println(message);
				//Serial.println(payload);
			}
			else if(String(msgType) == "mqtt")
			{
				if(mqttConnected != true)
				{
					Serial.println("Received serial mqtt data but we are not connected to the server");
				}
				else
				{
					Serial.println("Received serial mqtt data");
					mqtt.publish(const_cast<char*>(message),const_cast<char*>(payload));
				}
				}
				else
				{
					Serial.println("Received serial uncaught data");
				}
				Serial.println(content);
			}
		} 
	}

void setup()  
{
  pinMode(WIFI_LED_PIN, OUTPUT);
  pinMode(MQTT_LED_PIN, OUTPUT);
  digitalWrite(WIFI_LED_PIN, LOW);
  digitalWrite(MQTT_LED_PIN, LOW);
  // Serial
  Serial.begin(9600);
  delay(1000);
  //while ( !Serial ){ ; }         // wait for serial port to connect. Needed for native USB port only
  // Serial2
  nodemcu.begin(BAUD_RATE);
  Serial.println("Serial started ");
  sendSerialData("info","Connecting to WiFi","");
  // Connect to Wi-Fi
  network.connect();
  digitalWrite(WIFI_LED_PIN, HIGH);
  sendSerialData("info", "Connected to WiFi", network.localAddress().toString());
  delay(2000);
  sendSerialData("info", "Connecting to mqtt", "");
  if(mqtt.connect(mqtt_username, mqtt_password))
  {
    sendSerialData("info", "Connected to mqtt", "");
    // Subscribe to mqtt messages
    mqttSubscribe(roomID);
    sendSerialData("info", "Subscribed to mqtt", "");
    digitalWrite(MQTT_LED_PIN, HIGH);
    mqttConnected = true;
  }
}

void loop() 
{
  // check if we are connected to WiFi
  if(network.status( ) != WL_CONNECTED)
  {
    //digitalWrite(MQTT_LED_PIN, LOW);
    //sendSerialData("info","Connecting to WiFi","");
    // Check wifi status
    network.check();
    //digitalWrite(MQTT_LED_PIN, HIGH);
    //sendSerialData("info", "Connected to WiFi", network.localAddress().toString());
  }
  if(!mqtt.isConnected())
  {
    mqttConnected = false;
    digitalWrite(MQTT_LED_PIN, LOW);
    sendSerialData("info", "Connecting to mqtt", "");
    // Reconnected to mqtt server
    if(mqtt.connect( mqtt_username, mqtt_password))
    {
      sendSerialData("info", "Connected to mqtt", "");
      // Subscribe to mqtt messages
      mqttSubscribe(roomID);
      sendSerialData("info", "Subscribed to mqtt", "");
      digitalWrite(MQTT_LED_PIN, HIGH);
      mqttConnected = true;
    }
  }
  // Loop mqtt client
  mqtt.loop();
  // Read incoming msgs from Serial2
  readSerialData( );
}