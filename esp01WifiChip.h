#include "/home/irony/Arduino/Globals/credentials.h"

// Net tools
#include <NetTools.h>

// Json library
#include <ArduinoJson.h>

// Software serial
#include "SoftwareSerial.h"

// System ID
String roomID = "kroom";
String componentID = "air-sensors";


const String mqttClientID = roomID + "-" + componentID;
const String mqttTopic = roomID + "/" + componentID;

// Serial debugging
//#define START_SERIAL_DEBUG 1

  // Esp-01
  //#define RXD2 3
  //#define TXD2 1

  #define RXD2 2
  #define TXD2 0
  
  //#define WIFI_LED_PIN 2
  //#define MQTT_LED_PIN 0  
  
  // Esp32 dev kit
  //#define RXD2 16
  //#define TXD2 17
  //#define WIFI_LED_PIN 18
  //#define MQTT_LED_PIN 19
  
  #define BAUD_RATE 4800

  // Serial debugging
  #ifndef START_SERIAL_DEBUG
    #define START_SERIAL_DEBUG 0
  #endif

// Variables
//const String mqttClientID = roomID + "-display";
//const String mqttTopic = roomID + "/display";
String content = "";
bool mqttConnected = false;

// Init software serial
SoftwareSerial nodemcu(RXD2, TXD2); //RX, TX

void sendSerialData(const String& dataType, const String& message, const String& payload)
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
    /*if(String( topic ) == roomID + "/relay-controller-restart")
    {
      delay(1000);
      ESP.restart();
    }*/
  }
}

// Wifi tools
NetTools::WIFI network(ssid, password);

// Mqtt tools
NetTools::MQTT mqtt(mqttClientID, mqtt_server, mqtt_callback);

void mqttSubscribe(const String& roomID)
{
  Serial.println("Subscribing to mqtt messages");
  if (componentID == "air-sensors")
  {
    mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-backlight").c_str()));
    mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-restart").c_str()));
  }
  else if (componentID == "relay-controller")
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
    mqtt.subscribe(const_cast<char*>(String(roomID + "/air-values").c_str()));
    mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-update-interval").c_str()));
    mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-display-backlight").c_str()));
    mqtt.subscribe(const_cast<char*>(String(roomID + "/" + componentID + "-restart").c_str()));
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
            if(String(message) == "wifi-restart")
            {
              ESP.restart();
            }
            
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
  /*if (!START_SERIAL_DEBUG)
  {
    pinMode(WIFI_LED_PIN, OUTPUT);
    pinMode(MQTT_LED_PIN, OUTPUT);
    digitalWrite(WIFI_LED_PIN, LOW);
    digitalWrite(MQTT_LED_PIN, LOW);
  
  }
  else
  {
    Serial.begin(9600, SERIAL_8N1, SERIAL_TX_ONLY);
    Serial.begin(9600);
    while ( !Serial ){ ; }         // wait for serial port to connect. Needed for native USB port only
  }*/
  if (RXD2 != 3)
  {
    Serial.begin(9600);
  }
  nodemcu.begin(BAUD_RATE);
  delay(1000);
  Serial.println("Serial started ");
  sendSerialData("info","Connecting to WiFi","");
  network.connect();
  /*if (!START_SERIAL_DEBUG)
  {
    digitalWrite(WIFI_LED_PIN, HIGH);
  }*/
  sendSerialData("info", "Connected to WiFi", network.localAddress().toString());
  delay(2000);
  sendSerialData("info", "Connecting to mqtt", "");
  if (mqtt.connect(mqtt_username, mqtt_password))
  {
    sendSerialData("info", "Connected to mqtt", "");
    mqttSubscribe(roomID);
    sendSerialData("info", "Subscribed to mqtt", "");
    /*if (!START_SERIAL_DEBUG)
    {
      digitalWrite(MQTT_LED_PIN, HIGH);
    }*/
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
   /*if (!START_SERIAL_DEBUG)
    {
      digitalWrite(MQTT_LED_PIN, LOW);
    }*/
    sendSerialData("info", "Connecting to mqtt", "");
    if(mqtt.connect(mqtt_username, mqtt_password))
    {
      sendSerialData("info", "Connected to mqtt", "");
      mqttSubscribe(roomID);
      sendSerialData("info", "Subscribed to mqtt", "");
      /*if (!START_SERIAL_DEBUG)
      {
        digitalWrite(MQTT_LED_PIN, HIGH);
      }*/
      mqttConnected = true;
    }
  }
  mqtt.loop();
  readSerialData( );
}