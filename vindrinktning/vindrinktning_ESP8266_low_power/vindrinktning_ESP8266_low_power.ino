#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

#define MQTT_ENABLED 1

// Connect to D2 on NodeMCU
#define rxPin 4
// Unused TX Ppin
#define txPin 13

const char* ssid     = "Your-WIFI-SSID";
const char* password = "Your-WIFI-Password";
const char* MQTT_BROKER = "mqtt-broker-ip";
const int MQTT_PORT = 1883;
const char* MQTT_PUB_TOPIC = "sensor/Vindrinktning/PM1006";
const char * MQTT_CLIENT_ID = "Vindrinktning"

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

bool firstBoot = false;
const int awakeTimeS_firstBoot = 5*60;

// Time to sleep (in seconds):
const int sleepTimeS = 60;

SoftwareSerial sSerial(rxPin, txPin);

char rxBuf[32];
int rxPos = 0;
uint16_t ppm2_5;
uint16_t ppm1_0;
uint16_t ppm10;

void start_wifi(void)
{
  Serial.printf("Connecting to %s\n", ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf(".");
  }

  Serial.printf("\nWiFi connected\n");
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
}

void stop_wifi(void)
{
  if(WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
  }
  WiFi.mode(WIFI_OFF);
}

void setup_mqtt(void)
{
  mqtt_client.setServer(MQTT_BROKER, 1883);
}

void mqtt_reconnect(void) {
  while (!mqtt_client.connected()) {
    Serial.printf("MQTT connecting...\n");
    if (!mqtt_client.connect("ESP8266Client")) {
      Serial.printf("failed, rc=%d, retrying in 5 seconds\n", mqtt_client.state());
      delay(5000);
    }
  }
}

void start_mqtt(void)
{
  Serial.printf("Connecting to MQTT-Broker\n");
  if(!mqtt_client.connected()) {
    mqtt_reconnect();
  }
  mqtt_client.loop();
}

void stop_mqtt(void)
{
  if(mqtt_client.connected()) {
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
  Serial.printf("Waiting for new Data\n");
  sSerial.begin(9600);

  while(1) {
    static unsigned long received_tick = 0;
    // send data only when you receive data:
    if (sSerial.available() > 0) {
      rxBuf[rxPos] = sSerial.read();
      rxPos++;
      received_tick = millis();
    }else if(received_tick + 50 < millis() && rxPos == 20) {
      Serial.printf("RX: ");
      for(int i=0;i<rxPos;i++) {
        Serial.printf("%02X ", rxBuf[i]);
      }
      
      Serial.printf("\n");
    
      ppm2_5 = rxBuf[5]<<8 | rxBuf[6];
      ppm1_0 = rxBuf[9]<<8 | rxBuf[10];
      ppm10 = rxBuf[13]<<8 | rxBuf[14];
      Serial.printf("PM2.5 :%4d\n", ppm2_5);
      Serial.printf("PM1.0 :%4d\n", ppm1_0);
      Serial.printf("PM10  :%4d\n", ppm10);
      return;
    }else if(received_tick + 50 < millis())
    {
      // corrupted data received
      rxPos = 0;
    }
  }
}

void check_wakeup_reason(void) {
  Serial.printf("Wakeup by: ");
  
  switch(ESP.getResetInfoPtr()->reason) {
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

void setup() {
  Serial.begin(9600);
  Serial.printf("\nIKEA Vindrinktning Logger\n");

  check_wakeup_reason();

  stop_wifi();
  setup_mqtt();

  if(firstBoot == true) {
    start_wifi();
  }else {
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
  // only enter loop to provide OTA option

  if(millis()/1000 > awakeTimeS_firstBoot) {
    stop_wifi();
    enter_DeepSleep(sleepTimeS / 1000);
  }
}
