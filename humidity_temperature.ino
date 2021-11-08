#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "DHTesp.h"
#include "ThingSpeak.h"

#ifndef STASSID
#define STASSID "Samsung A52s"
#define STAPSK  "k!k!l@l@"
#endif

DHTesp dht;
const char* ssid = STASSID;
const char* pass = STAPSK;
WiFiClient  client;

unsigned long myChannelNumber = 1562422;
const char * myWriteAPIKey = "CV3XBW6YVXDY5VPC";

ComfortState cf;
String comfortStatus;
String perception;

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  dht.setup(0, DHTesp::DHT11);
  //  dht.setup(0, DHTesp::DHT22);
  ThingSpeak.begin(client);
}

void loop() {
  delay(dht.getMinimumSamplingPeriod());

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  float cr = dht.getComfortRatio(cf, temperature, humidity, false);

  switch (cf) {
    case Comfort_OK:
      comfortStatus = "Comfort_OK";
      break;
    case Comfort_TooHot:
      comfortStatus = "Comfort_TooHot";
      break;
    case Comfort_TooCold:
      comfortStatus = "Comfort_TooCold";
      break;
    case Comfort_TooDry:
      comfortStatus = "Comfort_TooDry";
      break;
    case Comfort_TooHumid:
      comfortStatus = "Comfort_TooHumid";
      break;
    case Comfort_HotAndHumid:
      comfortStatus = "Comfort_HotAndHumid";
      break;
    case Comfort_HotAndDry:
      comfortStatus = "Comfort_HotAndDry";
      break;
    case Comfort_ColdAndHumid:
      comfortStatus = "Comfort_ColdAndHumid";
      break;
    case Comfort_ColdAndDry:
      comfortStatus = "Comfort_ColdAndDry";
      break;
    default:
      comfortStatus = "Comfort_Unknown";
      break;
  };

  uint8_t cp = dht.computePerception(temperature, humidity, false);

  switch (cp) {
    case 0:
      perception = "Perception_Dry";
      break;
    case 1:
      perception = "Perception_VeryComfortable";
      break;
    case 2:
      perception = "Perception_Comfortable";
      break;
    case 3:
      perception = "Perception_Ok";
      break;
    case 4:
      perception = "Perception_Uncomfortable";
      break;
    case 5:
      perception = "Perception_QuiteUncomfortable";
      break;
    case 6:
      perception = "Perception_VeryUncomfortabl";
      break;
    case 7:
      perception = "Perception_SevereUncomfortable";
      break;
    default:
      perception = "Perception_Unknown";
      break;
  };

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(STASSID);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass);
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
  }

  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, cr);
  ThingSpeak.setField(4, cp);
  ThingSpeak.setStatus(comfortStatus + ", " + perception);

  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x == 200) {
    Serial.println("Channel update successful.");
  }
  else {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }

  for (uint8_t i = 1; i <= 20; i++) {
    ArduinoOTA.handle();
    delay(1000);
  }
}
