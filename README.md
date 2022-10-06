# arduino-grow-system

## Relay Controller
```
// System ID
String roomID = "your-system-id";

#include "RelayController.h"
```

## Relay Controller WiFi
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
