#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "DHTesp.h"
#include "ThingSpeak.h"

DHTesp dht;
WiFiClient client;
const uint8_t dhtPin = 14;
const uint8_t dhtGndPin = 12;
const uint8_t interval = 20;
const char* ssid = "IPB";
const char* pass = "di8ITipb";
unsigned long myChannelNumber = 1567804;
const char * myWriteAPIKey = "BRKMKOUAARRUJSZB";
ComfortState cf;
String comfort;
String perception;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nBooting");
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
  dht.setup(dhtPin, DHTesp::DHT22);
  ThingSpeak.begin(client);
  pinMode(dhtGndPin, OUTPUT);
  digitalWrite(dhtGndPin, HIGH);
  delay(1000);
  digitalWrite(dhtGndPin, LOW);
  Serial.println("Ready");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to Wifi");
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass);
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nWifi connected.");
  }

  for (uint8_t i = 1; i <= interval; i++) {
    ArduinoOTA.handle();
    delay(1000);
  }

  delay(dht.getMinimumSamplingPeriod());

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  float cr = dht.getComfortRatio(cf, temperature, humidity, false);

  switch (cf) {
    case Comfort_OK:
      comfort = "OK";
      break;
    case Comfort_TooHot:
      comfort = "Too Hot";
      break;
    case Comfort_TooCold:
      comfort = "Too Cold";
      break;
    case Comfort_TooDry:
      comfort = "Too Dry";
      break;
    case Comfort_TooHumid:
      comfort = "Too Humid";
      break;
    case Comfort_HotAndHumid:
      comfort = "Hot And Humid";
      break;
    case Comfort_HotAndDry:
      comfort = "Hot And Dry";
      break;
    case Comfort_ColdAndHumid:
      comfort = "Cold And Humid";
      break;
    case Comfort_ColdAndDry:
      comfort = "Cold And Dry";
      break;
    default:
      comfort = "Unknown";
      break;
  };

  uint8_t cp = dht.computePerception(temperature, humidity, false);

  switch (cp) {
    case 0:
      perception = "Dry";
      break;
    case 1:
      perception = "Very Comfortable";
      break;
    case 2:
      perception = "Comfortable";
      break;
    case 3:
      perception = "OK";
      break;
    case 4:
      perception = "Uncomfortable";
      break;
    case 5:
      perception = "Quite Uncomfortable";
      break;
    case 6:
      perception = "Very Uncomfortable";
      break;
    case 7:
      perception = "Severe Uncomfortable";
      break;
    default:
      perception = "Unknown";
      break;
  };

  char sts[255] = {0};
  char Log[255] = {0};
  IPAddress ip = WiFi.localIP();
  sprintf(Log, "Local IP Address: %u.%u.%u.%u\nTemperature: %0.2f Celcius\nHumidity: %0.2f %c\nComfort: %s\nPerception: %s\n\n",
          ip[0], ip[1], ip[2], ip[3], temperature, humidity, '%', &comfort[0], &perception[0]);
  sprintf(sts, "Comfort: %s || Perception: %s || Local IP Address: %u.%u.%u.%u", &comfort[0], &perception[0], ip[0], ip[1], ip[2], ip[3]);
  Serial.print(Log);

  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, cr);
  ThingSpeak.setField(4, cp);
  ThingSpeak.setStatus(sts);

  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x == 200) {
    Serial.print("Channel update successful\n\n");
  }
  else {
    Serial.print("Problem updating channel. HTTP error code " + String(x) + "\n\n");
  }
}
