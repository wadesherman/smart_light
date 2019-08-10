#define FASTLED_ALLOW_INTERRUPTS 0

#include <noise.h>
#include <bitswap.h>
#include <fastspi_types.h>
#include <pixelset.h>
#include <fastled_progmem.h>
#include <led_sysdefs.h>
#include <hsv2rgb.h>
#include <fastled_delay.h>
#include <colorpalettes.h>
#include <color.h>
#include <fastspi_ref.h>
#include <fastspi_bitbang.h>
#include <controller.h>
#include <fastled_config.h>
#include <colorutils.h>
#include <chipsets.h>
#include <pixeltypes.h>
#include <fastspi_dma.h>
#include <fastpin.h>
#include <fastspi_nop.h>
#include <platforms.h>
#include <lib8tion.h>
#include <cpp_compat.h>
#include <fastspi.h>
#include <FastLED.h>
#include <dmx.h>
#include <power_mgt.h>

#include <Arduino.h>
#include <dummy.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "secrets.h"

#define RED_LED 13
#define LED_ON HIGH
#define LED_OFF LOW

#define HW_UART_SPEED                115200L

const char* S = MQTT_ID STATE_TOPIC;
const char* C = MQTT_ID COLOR_TOPIC;
const char* B = MQTT_ID BRIGHTNESS_TOPIC;
const char* P = MQTT_ID PRESETS_TOPIC;

const char* WIFI_HOSTNAME = "ESP_" MQTT_ID;

static WiFiClient network;
PubSubClient client(network);

void(* resetFunc) (void) = 0;

FASTLED_USING_NAMESPACE
#define DATA_PIN    12
#define LED_TYPE    NEOPIXEL
#define NUM_LEDS 20
#define FRAMES_PER_SECOND  30
CRGB leds[NUM_LEDS];

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { solid, rainbow, confetti, bpm, waves };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

int brightness;
int color[3];
bool state;
char* preset;

void setBrightness() {
  Serial.print("setBrightness: ");
  Serial.println(brightness);
  FastLED.setBrightness(brightness);
}

void setState() {
  Serial.print("setState: ");
  Serial.println(state);

  if(state == true){
    FastLED.setBrightness(brightness);
  } else {
    FastLED.setBrightness(0);
  }
}

void setColor() {
  Serial.print("setColor:");
  Serial.print(" H:");
  Serial.print(color[0]);
  Serial.print(" S:");
  Serial.print(color[1]);
  Serial.print(" V:");
  Serial.print(color[2]);
  Serial.println("");
  gCurrentPatternNumber = 0;
}

void setPreset() {
  Serial.print("Preset: ");
  Serial.println(preset);
  if(strcmp(preset, "rainbow") == 0) {
    gCurrentPatternNumber = 1;
  }
  
  else if(strcmp(preset, "confetti") == 0) {
    gCurrentPatternNumber = 2;
  }

  else if(strcmp(preset, "bpm") == 0) {
    gCurrentPatternNumber = 3;
  }
  
  else if(strcmp(preset, "light") == 0) {
    color[0] = 58;
    color[1] = 28;
    color[2] = 255;
    setColor();
  }

  
  else if(strcmp(preset, "nightlight") == 0) {
    color[0] = 53;
    color[1] = 158;
    color[2] = 255;
    setColor();
  }
  
  else if(strcmp(preset, "waves") == 0) {
    gCurrentPatternNumber = 4;
  }

  else {

  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  char* m = (char *)payload;
  payload[length] = '\0';
  
  // STATE
  if (strcmp(topic, S) == 0) {
    if (strcmp(m, "on") == 0) {
      state = true;
    } else {
      state = false;
    }

    setState();

   // BRIGHTNESS
  } else if (strcmp(topic, B) == 0) {
    float t_b = atof(m);
    brightness = t_b * 100;
    brightness = map(brightness, 0, 100, 0, 255);
    
    setBrightness();

  // COLOR
  } else if (strcmp(topic, C) == 0) {
    Serial.println(m);
    int h, s, v;
    float f_h, f_s, f_v;
    
    if (sscanf(m, "%d,%d,%d", &h, &s, &v) == 3) {
      color[0] = map(h,0,360,0,255);
      color[1] = map(s,0,100,0,255);
      color[2] = map(v,0,100,0,255);
      setColor();
    } else if (sscanf(m, "%f,%f,%f", &f_h, &f_s, &f_v) == 3) {
      color[0] = map(int(f_h),0,360,0,255);
      color[1] = map(int(f_s),0,100,0,255);
      color[2] = map(int(f_v),0,100,0,255);
      setColor();
    } else {
      Serial.println("error :(");
    }

  // PRESETS
  } else if (strcmp(topic, P) == 0) {
    preset = m;
    setPreset();
  } else {
    Serial.println("Topic not recognized?");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    digitalWrite(RED_LED, LED_ON);
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_ID)) {
      digitalWrite(RED_LED, LED_OFF);
      Serial.println("connected");
      
      // subscribe
      client.subscribe(S);
      client.subscribe(B);
      client.subscribe(C);
      client.subscribe(P);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


// ============== Setup all objects ============================================
void setup() {
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LED_OFF);
  
  // Setup hardware serial for logging
  Serial.begin(HW_UART_SPEED);
  while (!Serial);

  // Setup WiFi network
  WiFi.mode(WIFI_STA);
  //WiFi.hostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PSK);
  Serial.println("Connecting to WiFi");

  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);

  FastLED.addLeds<LED_TYPE,DATA_PIN>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
}

// ============== Main loop ====================================================
void loop() {
  //wifi client
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(RED_LED, LED_ON);
    delay(250);
    digitalWrite(RED_LED, LED_OFF);
    delay(250);
    Serial.print("."); 
  }
  //mqtt client
  if (!client.connected()) {
    reconnect();
  }
  
  //random16_add_entropy( random());
  
  gPatterns[gCurrentPatternNumber]();
  FastLED.show();  
  FastLED.delay(1000/FRAMES_PER_SECOND); 
  EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  
  client.loop();
}

void solid()
{
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = CHSV(color[0], color[1], color[2]);
  }
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}


void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 120;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void waves()
{
  int h = map(sin8(gHue),0,255,125,160);
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(h,255,255);
  }
}
