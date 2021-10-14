#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <ArduinoOTA.h>

#include "config.h"

// Use Software serial RX on GPIO2 (D4 on NodeMCU), select any unused pin for TX
#define rxPin 2
#define txPin 13

const char* sta_ssid     = WIFI_STATION_SSID;
const char* sta_password = WIFI_STATION_PASSWORD;
const char* ap_ssid      = WIFI_AP_SSID;
const char* ap_password  = WIFI_AP_PASSWORD;

SoftwareSerial sSerial(rxPin, txPin);
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

bool firstBoot = false;
const int awakeTimeS_firstBoot = 5 * 60;

// Time to sleep (in seconds):
const int sleepTimeS = 60;

uint16_t ppm2_5;
uint16_t ppm1_0;
uint16_t ppm10;

bool ota_in_progress = false;

void start_wifi(void)
{
  Serial.printf("Connecting to %s\n", sta_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(sta_ssid, sta_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf(".");
  }

  Serial.printf("\nWiFi connected\n");
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
}

void stop_wifi(void)
{
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
  }
  WiFi.mode(WIFI_OFF);
}

void setup_mqtt(void)
{
  mqtt_client.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
}

void mqtt_reconnect(void) {
  while (!mqtt_client.connected()) {
    Serial.printf("MQTT connecting...\n");
    if (!mqtt_client.connect(MQTT_CLIENT_ID)) {
      Serial.printf("failed, rc=%d, retrying in 5 seconds\n", mqtt_client.state());
      delay(5000);
    }
  }
}

void start_mqtt(void)
{
  Serial.printf("Connecting to MQTT-Broker: %s:%d\n", MQTT_BROKER_IP, MQTT_BROKER_PORT);
  if (!mqtt_client.connected()) {
    mqtt_reconnect();
  }
  mqtt_client.loop();
}

void stop_mqtt(void)
{
  if (mqtt_client.connected()) {
    mqtt_client.disconnect();
  }
}

void send_mqtt(uint16_t ppm2_5, uint16_t ppm1_0, uint16_t ppm10)
{
  char payload[32];
  sprintf(payload, "%d,%d,%d", ppm2_5, ppm1_0, ppm10);
  mqtt_client.publish(MQTT_PUB_TOPIC, payload);
  mqtt_client.loop(); // process MQTT queue
}

void enter_DeepSleep(int time_S) {
  Serial.printf("Enter deep sleep for %ds\n", time_S);
  ESP.deepSleep(time_S * 1000000);
  yield();
}

void PM1006_Readback(void) {
  static char rxBuf[32];
  static int rxPos = 0;

  Serial.printf("Waiting for new Data\n");
  sSerial.begin(9600);

  while (1) {
    static unsigned long received_tick = 0;
    // send data only when you receive data:
    if (sSerial.available() > 0) {
      rxBuf[rxPos] = sSerial.read();
      rxPos++;
      received_tick = millis();
    } else if (received_tick + 50 < millis() && rxPos == 20) {
      Serial.printf("RX: ");
      for (int i = 0; i < rxPos; i++) {
        Serial.printf("%02X ", rxBuf[i]);
      }

      Serial.printf("\n");

      ppm2_5 = rxBuf[5] << 8 | rxBuf[6];
      ppm1_0 = rxBuf[9] << 8 | rxBuf[10];
      ppm10 = rxBuf[13] << 8 | rxBuf[14];
      Serial.printf("PM2.5 :%4d\n", ppm2_5);
      Serial.printf("PM1.0 :%4d\n", ppm1_0);
      Serial.printf("PM10  :%4d\n", ppm10);
      return;
    } else if (received_tick + 50 < millis())
    {
      // corrupted data received
      rxPos = 0;
    }
  }
}

void check_wakeup_reason(void) {
  Serial.printf("Wakeup by: ");

  switch (ESP.getResetInfoPtr()->reason) {
    case REASON_DEFAULT_RST:
      Serial.print("Default\n");
      firstBoot = true;
      break;
    case REASON_WDT_RST:
      Serial.printf("Watchdog\n");
      break;
    case REASON_EXCEPTION_RST:
      Serial.printf("Exception\n");
      break;
    case REASON_SOFT_WDT_RST:
      Serial.printf("Soft Watchdog\n");
      break;
    case REASON_SOFT_RESTART:
      Serial.printf("Soft Restart\n");
      break;
    case REASON_DEEP_SLEEP_AWAKE:
      Serial.printf("Deep sleep\n");
      break;
    case REASON_EXT_SYS_RST:
      Serial.printf("External System Reset\n");
      firstBoot = true;
      break;
    default:
      Serial.printf("Undefined\n");
      break;
  }
}

void setup_OTA(void) {

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.printf("Start updating %s\n", type.c_str());
    ota_in_progress = true;
  });
  ArduinoOTA.onEnd([]() {
    Serial.printf("End\n");
    ota_in_progress = false;
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
    ota_in_progress = true;
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.printf("Auth Failed\n");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.printf("Begin Failed\n");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.printf("Connect Failed\n");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.printf("Receive Failed\n");
    } else if (error == OTA_END_ERROR) {
      Serial.printf("End Failed\n");
    }
  });
  ArduinoOTA.begin();

  Serial.printf("OTA Handler started at % s: % d\n", OTA_HOSTNAME, OTA_PORT);
}

void setup() {
  Serial.begin(9600);
  Serial.printf("\nIKEA Vindrinktning Logger\n");

  check_wakeup_reason();

  stop_wifi();
  setup_mqtt();

  if (firstBoot == true) {
    start_wifi();
    setup_OTA();
  } else {
    PM1006_Readback();

    start_wifi();

    start_mqtt();
    send_mqtt(ppm2_5, ppm1_0, ppm10);
    stop_mqtt();

    stop_wifi();

    enter_DeepSleep(sleepTimeS - millis() / 1000);
  }
}

void loop() {
  // only enter loop to support OTA
  ArduinoOTA.handle();

  if (millis() / 1000 > awakeTimeS_firstBoot && !ota_in_progress) {
    stop_wifi();
    enter_DeepSleep(sleepTimeS);
  }
}
