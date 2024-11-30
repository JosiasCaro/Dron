#include "AsyncCam.hpp"
#include <WiFi.h>

static const char* WIFI_SSID = "Speedy-Fibra-1E806C";
static const char* WIFI_PASS = "398cC577K4Rc75B5AfF5";

IPAddress ip(192,168,1,200);     
IPAddress gateway(192,168,1,1);   
IPAddress subnet(255,255,255,0);  

esp32cam::Resolution initialResolution;
esp32cam::Resolution currentResolution;

AsyncWebServer server(80);
const int ledPin = 4;

void blinkLed() {
  for (int i = 0; i < 3; ++i) {
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    delay(500);
  }
}

void
setup()
{
  Serial.begin(115200);
  Serial.println();
  delay(2000);

  pinMode(ledPin, OUTPUT);
  blinkLed();

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi failure");
    delay(5000);
    ESP.restart();
  }
  Serial.println("WiFi connected");

  {
    using namespace esp32cam;

    initialResolution = Resolution::find(1024, 768);
    currentResolution = initialResolution;

    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(initialResolution);
    cfg.setJpeg(80);

    bool ok = Camera.begin(cfg);
    if (!ok) {
      Serial.println("camera initialize failure");
      delay(5000);
      ESP.restart();
    }
    Serial.println("camera initialize success");
  }

  Serial.println("camera starting");
  Serial.print("http://");
  Serial.println(WiFi.localIP());

  addRequestHandlers();
  server.begin();
}

void
loop()
{
  // esp32cam-asyncweb.h depends on FreeRTOS task API including vTaskDelete, so you must have a
  // non-zero delay in the loop() function; otherwise, FreeRTOS kernel memory cannot be freed
  // properly and the system would run out of memory.
  delay(1);
}
