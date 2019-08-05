#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <dummy.h>
#include <WiFi.h>
#include <MqttClient.h>

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

static MqttClient *mqtt = NULL;
static WiFiClient network;

// ============== Object to supply system functions ============================
class System: public MqttClient::System {
public:

	unsigned long millis() const {
		return ::millis();
	}

	void yield(void) {
		::yield();
	}
};


void(* resetFunc) (void) = 0;

#define LED_PIN    6
#define LED_COUNT 20
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


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

	// Setup MqttClient
	MqttClient::System *mqttSystem = new System;
	MqttClient::Logger *mqttLogger = new MqttClient::LoggerImpl<HardwareSerial>(Serial);
	MqttClient::Network * mqttNetwork = new MqttClient::NetworkClientImpl<WiFiClient>(network, *mqttSystem);
	//// Make 128 bytes send buffer
	MqttClient::Buffer *mqttSendBuffer = new MqttClient::ArrayBuffer<128>();
	//// Make 128 bytes receive buffer
	MqttClient::Buffer *mqttRecvBuffer = new MqttClient::ArrayBuffer<128>();
	//// Allow up to 2 subscriptions simultaneously
	MqttClient::MessageHandlers *mqttMessageHandlers = new MqttClient::MessageHandlersImpl<4>();
	//// Configure client options
	MqttClient::Options mqttOptions;
	////// Set command timeout to 10 seconds
	mqttOptions.commandTimeoutMs = 10000;
	//// Make client object
	mqtt = new MqttClient(
		mqttOptions, *mqttLogger, *mqttSystem, *mqttNetwork, *mqttSendBuffer,
		*mqttRecvBuffer, *mqttMessageHandlers
	);

  //strip.begin();
  //strip.show();
  //strip.setBrightness(50);
  Serial.print("loop() running on core ");
  Serial.println(xPortGetCoreID());
}

// ============== Main loop ====================================================
void loop() {
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(RED_LED, LED_ON);
    delay(250);
    digitalWrite(RED_LED, LED_OFF);
    delay(250);
    Serial.print("."); 
  }
  Serial.println("Connected to WiFi");
  Serial.println("IP: " + WiFi.localIP().toString());
  
	// Check connection status
	if (!mqtt->isConnected()) {
		// Close connection if exists
		network.stop();
		// Re-establish TCP connection with MQTT broker
		Serial.println("Connecting to MQTT Broker");
		network.connect("mq.casadeoso.com", 1883);
		if (!network.connected()) {
			Serial.println("Can't establish the MQTT TCP connection");
      digitalWrite(RED_LED, LED_ON);
			delay(5000);
      resetFunc();
		}
		// Start new MQTT connection
		MqttClient::ConnectResult connectResult;
		// Connect
		{
			MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
			options.MQTTVersion = 4;
			options.clientID.cstring = (char*)MQTT_ID;
			options.cleansession = true;
			options.keepAliveInterval = 15; // 15 seconds
			MqttClient::Error::type rc = mqtt->connect(options, connectResult);
			if (rc != MqttClient::Error::SUCCESS) {
				Serial.println("Connection error");
        digitalWrite(RED_LED, LED_ON);
				return;
			}
		}
    mqtt->subscribe(C, MqttClient::QOS0, processColor);
    mqtt->subscribe(P, MqttClient::QOS0, processPresets);
    mqtt->subscribe(S, MqttClient::QOS0, processState);
    mqtt->subscribe(B, MqttClient::QOS0, processBrightness);
	} 

	// Idle for 30 seconds
	mqtt->yield(30000L);
	
}

void publish(char* payload, const char* topic) {
  MqttClient::Message message;
    message.qos = MqttClient::QOS0;
    message.retained = true;
    message.dup = false;
    message.payload = (void*) payload;
    message.payloadLen = strlen(payload);
    
    mqtt->publish(topic, message);
}

// ============== Subscription callbacks =======================================
void processState(MqttClient::MessageData& md) {
  const MqttClient::Message& msg = md.message;
  char payload[msg.payloadLen + 1];
  memcpy(payload, msg.payload, msg.payloadLen);
  payload[msg.payloadLen] = '\0';
  
  if(strcmp(payload,"on") == 0) {
    Serial.println("on");
  } else {
    Serial.println("off");
  }
}

void processBrightness(MqttClient::MessageData& md) {
  const MqttClient::Message& msg = md.message;
  char payload[msg.payloadLen + 1];
  memcpy(payload, msg.payload, msg.payloadLen);
  payload[msg.payloadLen] = '\0';
  //int brightness = map(atoi(payload), 0, 100, 0, 255);
  
  Serial.println(payload);
  //strip.setBrightness(brightness);
}

void processColor(MqttClient::MessageData& md) {
  const MqttClient::Message& msg = md.message;
  char payload[msg.payloadLen + 1];
  memcpy(payload, msg.payload, msg.payloadLen);
  payload[msg.payloadLen] = '\0';
 // strip.fill(strtol(payload, 0, 16)); //this might work.
 Serial.println(payload);
}

void processPresets(MqttClient::MessageData& md) {
  const MqttClient::Message& msg = md.message;
  char payload[msg.payloadLen + 1];
  memcpy(payload, msg.payload, msg.payloadLen);
  payload[msg.payloadLen] = '\0';

  // implement this: https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/
  if(strcmp(payload,"nightlight") == 0) {
    Serial.println("nightlight");
  } else if (strcmp(payload,"light") == 0)  {
    Serial.println("light");
  } else if (strcmp(payload,"waves") == 0)  {
    Serial.println("waves");
   // colorWipe(strip.Color(0, 255, 255), 50);
  } else if (strcmp(payload,"rainbow") == 0)  {
    Serial.println("rainbow");
    //rainbow(10);
  }
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

void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}
