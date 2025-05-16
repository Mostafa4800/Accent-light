#include <WiFi.h>
#include <PubSubClient.h>
#include <FastLED.h>

// FastLED setup
#define LED_PIN     4
#define NUM_LEDS    150
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS  100

CRGB leds[NUM_LEDS];

// WiFi credentials
const char* ssid = "Next-Iot";
const char* password = "shin-pygmy-renal-Hahk";

// MQTT setup
const char* mqtt_server = "mqtt-plain.nextservices.dk";
WiFiClient espClient;
PubSubClient client(espClient);

// Trail color variable
CRGB trailColor = CRGB::Blue;  // Default trail color

// Function declarations
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void shootStar();

void setup() {
  Serial.begin(115200);

  // LED setup
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // MQTT setup
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';  // Null-terminate string
  String topicStr = String(topic);
  String payloadStr = String((char*)payload);

  Serial.println("Received topic: " + topicStr);
  Serial.println("Payload: " + payloadStr);

  if (topicStr == "Pairing/Server") {
    String response = "{ResponseTopic: Star3/Response, CommandTopic: Star3/Command, ID: 3, DeviceType: LedStar, Status: off}";
    client.publish("Pairing/Client", response.c_str());
  }

  if (topicStr == "Star3/Command") {
    // Support for #RRGGBBAA (8-digit hex with alpha)
    if (payloadStr.startsWith("#") && payloadStr.length() == 9) {
      unsigned long number = strtoul(&payloadStr[1], NULL, 16);
      uint8_t r = (number >> 24) & 0xFF;
      uint8_t g = (number >> 16) & 0xFF;
      uint8_t b = (number >> 8) & 0xFF;
      uint8_t a = number & 0xFF;
      float alpha = a / 255.0;

      // Simulate alpha by dimming RGB
      trailColor = CRGB(r * alpha, g * alpha, b * alpha);
      Serial.printf("Parsed hex RGBA: R=%d G=%d B=%d A=%d -> TrailColor: (%d, %d, %d)\n", r, g, b, a, (int)(r * alpha), (int)(g * alpha), (int)(b * alpha));
    }

    // Support for #RRGGBB (6-digit hex)
    else if (payloadStr.startsWith("#") && payloadStr.length() == 7) {
      long number = strtol(&payloadStr[1], NULL, 16);
      trailColor = CRGB((number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF);
      Serial.print("Updated trail color (RGB): ");
      Serial.println(payloadStr);
    }

    shootStar();
    client.publish("Star3/Response", "Shot a star");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32_Client")) {
      Serial.println("connected");
      client.subscribe("Pairing/Server");
      client.subscribe("Star3/Command");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void shootStar() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i > 0) leds[i - 1] = CRGB::Black; // Clear behind
    leds[i] = CRGB::White;                // Tip
    FastLED.show();
  }

  delay(100); // Pause at end

  // Second pass: back with white tip, trailColor behind
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    if (i < NUM_LEDS - 1) leds[i + 1] = trailColor; // Leave color trail
    leds[i] = CRGB::White;                          // Tip
    FastLED.show();
  }

  // Leave full trail behind
  fill_solid(leds, NUM_LEDS, trailColor);
  FastLED.show();
}
