# Arduino Grow System

Automated grow system arudino ide sketches

## Relay controller sketch
```
// System ID
String roomID = "your-system-id";

#include "mainController.h"
```

## Air sensors sketch
```
// System ID
String roomID = "your-system-id";

#include "airSensors.h"
```

## WiFi chip sketch
```
// Network credentials
const char* ssid = "wifi-ssid";
const char* password = "wifi-password";

// MQTT Broker:
const char *mqtt_server = "mqtt-server-address";
const char *mqtt_username = "mqtt-username";
const char *mqtt_password = "mqtt-password";

// System ID
String roomID = "your-system-id";

#include "wifi.h"
```

## Services
```
// Configue all wifi chips
mosquitto_pub -t "{systemID}/config-component-id" -m "1" -u "user" -P "pass"

// Restart component
mosquitto_pub -t "{systemID}/{air-sensors|main-controller|water-tester}-restart" -m "1" -u "user" -P "pass"

// Component screen backlight on/off
mosquitto_pub -t "{systemID}/{air-sensors|main-controller|water-tester}-display-backlight" -m "1|0" -u "user" -P "pass"
```