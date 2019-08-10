# Smart Light

### Requirements:
- ESP32
- Addressable LED strip that can be controlled by the FastLED Arduino library
- an appropriate power supply
- an MQTT broker

### Arduino Library Requirements (third party)
- [FastLED](https://github.com/FastLED/FastLED) 
- [PubSubClient](https://github.com/knolleary/pubsubclient/tree/master/src)

## How to:
1. Wire up your ESP32 to your leds
1. Install the library dependencies using the library manager in the Arduino IDE
1. Copy `secrets.h.example` -> `secrets.h` and fill out accordingly
1. Flash the code to your ESP

If all goes correctly, you should see the built-in LED on the ESP flash for a 
few seconds while it connects to your WiFi.  After that, it is ready to
receive commands over the MQTT protocol.  The device subscribes to 4 channels:
- `state` - on/off
- `brightness` - the overall brightness of the strip
- `color` - the color to use in solid_color mode
- `presets` - preset animations
---

## Channels
### State
#### MQTT Topic: `DEVICE_ID + STATE_TOPIC`
String: `on`|`off`

### Brightness
#### MQTT Topic: `DEVICE_ID + BRIGHTNESS_TOPIC`
Int: 0-100

### Color
#### MQTT Topic: `DEVICE_ID + COLOR_TOPIC`
String: `h,s,v`
  - h: int 0-360
  - s: int 0-100
  - v: int 0-100

### Presets
#### MQTT Topic: `DEVICE_ID + PRESET_TOPIC`
String: `rainbow`|`waves`|`confetti`|`light`|`nightlight`
