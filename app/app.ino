#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <dummy.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "secrets.h"

#define RED_LED 13
#define LED_ON HIGH
#define LED_OFF LOW

#define HW_UART_SPEED								115200L

const char* S = MQTT_ID STATE_TOPIC;
const char* C = MQTT_ID COLOR_TOPIC;
const char* B = MQTT_ID BRIGHTNESS_TOPIC;
const char* P = MQTT_ID PRESETS_TOPIC;

const char* WIFI_HOSTNAME = "ESP_" MQTT_ID;

static WiFiClient network;
PubSubClient client(network);

void(* resetFunc) (void) = 0;

#define LED_PIN    12
#define LED_COUNT 20
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

TaskHandle_t LightTask;
int brightness;
int color[3];
bool state;
char* preset;

void setBrightness(void* parameter) {
  Serial.print("setBrightness: ");
  Serial.println(brightness);
  
  strip.setBrightness(brightness);
  strip.show();
  while(true){
    vTaskDelay(100);
  }
}

void setState(void* parameter) {
  Serial.print("setState: ");
  Serial.println(state);

  if(state == true){
    strip.setBrightness(brightness);
    strip.show();
  } else {
    strip.setBrightness(0);
    strip.show();
  }
  
  while(true){
    vTaskDelay(100);
  }
}

void setColor(void* parameter) {
  Serial.print("setColor:");
  Serial.print(" R:");
  Serial.print(color[0]);
  Serial.print(" G:");
  Serial.print(color[1]);
  Serial.print(" B:");
  Serial.print(color[2]);
  Serial.println("");
  
  colorWipe(strip.Color(color[0], color[1], color[2]), 10);
  while(true){
    vTaskDelay(100);
  }
}

void setPreset(void* parameter) {
  Serial.print("Preset: ");
  Serial.println(preset);
  if(strcmp(preset, "rainbow") == 0) {
    while(true) {
      rainbow(10);
      //vTaskDelay(100);
    }
  }

  else if(strcmp(preset, "light") == 0) {
    strip.setBrightness(125);
    colorWipe(strip.Color(255, 255, 255), 10);
    while(true) {
      vTaskDelay(100);
    }
  }

  
  else if(strcmp(preset, "nightlight") == 0) {
    strip.setBrightness(75);
    colorWipe(strip.Color(255, 225, 225), 10);
    while(true) {
      vTaskDelay(100);
    }
  }
  
  else if(strcmp(preset, "waves") == 0) {
    while(true) {
      rainbow(10);
      vTaskDelay(100);
    }
  }

  else {
    while(true) {
      vTaskDelay(100);
    }
  }
}

void idleTask(void* parameter) {
  Serial.println("Idling on Setup");
  strip.begin();
  while(true) {
    vTaskDelay(100);
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
    vTaskDelete(LightTask);
     xTaskCreatePinnedToCore(
      setState, /* Function to implement the task */
      "LightTask", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &LightTask,  /* Task handle. */
      0); /* Core where the task should run */

   // BRIGHTNESS
  } else if (strcmp(topic, B) == 0) {
    float t_b = atof(m);
    brightness = t_b * 100;
    brightness = map(brightness, 0, 100, 0, 255);
    vTaskDelete(LightTask);
     xTaskCreatePinnedToCore(
      setBrightness, /* Function to implement the task */
      "LightTask", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &LightTask,  /* Task handle. */
      0); /* Core where the task should run */

  // COLOR
  } else if (strcmp(topic, C) == 0) {
    Serial.println(m);
    if (sscanf(m, "%d,%d,%d", &color[0], &color[1], &color[2]) == 3) {
       vTaskDelete(LightTask);
       xTaskCreatePinnedToCore(
        setColor, /* Function to implement the task */
        "LightTask", /* Name of the task */
        10000,  /* Stack size in words */
        NULL,  /* Task input parameter */
        0,  /* Priority of the task */
        &LightTask,  /* Task handle. */
        0); /* Core where the task should run */
    } else {
      Serial.println("error :(");
    }

  // PRESETS
  } else if (strcmp(topic, P) == 0) {
    preset = m;
    vTaskDelete(LightTask);
     xTaskCreatePinnedToCore(
      setPreset, /* Function to implement the task */
      "LightTask", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &LightTask,  /* Task handle. */
      0); /* Core where the task should run */
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

  xTaskCreatePinnedToCore(
      idleTask, /* Function to implement the task */
      "LightTask", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &LightTask,  /* Task handle. */
      0); /* Core where the task should run */
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

  client.loop();
}



void rainbow(int wait) {
  for(long firstPixelHue = 0; firstPixelHue < 3*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    delay(wait);
  }
}

void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;
  for(int a=0; a<30; a++) {
    for(int b=0; b<3; b++) {
      strip.clear();
      for(int c=b; c<strip.numPixels(); c += 3) {
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue));
        strip.setPixelColor(c, color);
      }
      strip.show();
      delay(wait);
      firstPixelHue += 65536 / 90;
    }
  }
}

void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}
