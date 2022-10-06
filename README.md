# Arduino Grow System

Automated grow system arudino ide sketches

## Relay controller sketch
```
// System ID
String roomID = "your-system-id";

#include "RelayController.h"
```

## Relay controller WiFi sketch
```
// System ID
String roomID = "your-system-id";

// Network credentials
const char* ssid = "wifi-ssid";
const char* password = "wifi-password";

// MQTT Broker:
const char *mqtt_server = "mqtt-server-address";
const char *mqtt_username = "mqtt-username";
const char *mqtt_password = "mqtt-password";

#include "RelayControllerWiFi.h"
```

## Air sensors sketch
```
// System ID
String roomID = "your-system-id";
String componentID = "air-sensors";

// use to calibrate this sensor
//#define CALIBRATE_MQ135 1

#include "airSensors.h"
```

## Air sensors WiFi sketch
```
// System ID
String roomID = "your-system-id";
String componentID = "air-sensors";

// Network credentials
const char* ssid = "wifi-ssid";
const char* password = "wifi-password";

// MQTT Broker:
const char *mqtt_server = "mqtt-server-address";
const char *mqtt_username = "mqtt-username";
const char *mqtt_password = "mqtt-password";

#include "esp01WiFi.h"
```