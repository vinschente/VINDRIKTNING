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
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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
    Serial.println("MQTT connecting...");
    if (!mqtt_client.connect("ESP8266Client")) {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void start_mqtt(void)
{
  Serial.println("Connecting to MQTT-Broker");
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
  Serial.print("Enter deep sleep for ");
  Serial.println(time_S);
  Serial.println("s");
  ESP.deepSleep(time_S * 1000000);
  yield();
}

void PM1006_Readback(void) {
  Serial.print("Waiting for new Data\n");
  sSerial.begin(9600);

  while(1) {
    static unsigned long received_tick = 0;
    // send data only when you receive data:
    if (sSerial.available() > 0) {
      //int incomingByte = sSerial.read();
      //Serial.println(incomingByte, HEX);
      rxBuf[rxPos] = sSerial.read();
      rxPos++;
      received_tick = millis();
    }else if(received_tick + 50 < millis() && rxPos == 20) {
      Serial.print("RX: ");
      for(int i=0;i<rxPos;i++) {
        Serial.print(rxBuf[i] < 16 ? "0" : "");
        Serial.print(rxBuf[i], HEX);
        Serial.print(" ");
      }
      
      Serial.println();
    
      ppm2_5 = rxBuf[5]<<8 | rxBuf[6];
      ppm1_0 = rxBuf[9]<<8 | rxBuf[10];
      ppm10 = rxBuf[13]<<8 | rxBuf[14];
      Serial.print("PM2.5 :");Serial.println(ppm2_5);
      Serial.print("PM1.0 :");Serial.println(ppm1_0);
      Serial.print("PM10  :");Serial.println(ppm10);
      return;
    }else if(received_tick + 50 < millis())
    {
      // corrupted data received
      rxPos = 0;
    }
  }
}

void setup() {
  // initialize serial:
  Serial.begin(9600);
  Serial.print("IKEA Vindrinktning Logger\n");

  stop_wifi();
  setup_mqtt();

  PM1006_Readback();

  start_wifi();
  
  start_mqtt();
  send_mqtt(ppm2_5, ppm1_0, ppm10);
  stop_mqtt();
  
  stop_wifi();
  
  enter_DeepSleep(sleepTimeS - millis() / 1000);
}

void loop() {
}
