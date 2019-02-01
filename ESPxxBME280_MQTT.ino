#include <BME280I2C.h>
#include <Wire.h>
#include <PubSubClient.h>

#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

#define SERIAL_BAUD 115200
#define TIME_TO_SLEEP 900 /*sec*/
#define uS_TO_S_FACTOR 1000000

/* Change me */
#define MQTT_REGISTRATION_NAME "MQTT_REGISTRATION_NAME_CHANGEME"
char* mqttName = "mqttName_CHANGEME";
char mqttFullName[80]; 
const char* ssid = "ssid_CHANGEME";
const char* password = "password_CHANGEME";
const char* mqtt_server = "mqtt_server_CHANGEME";

/*End Change me*/

BME280I2C bme;    // Default : forced mode, standby time = 1000 ms // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

WiFiClient espClient;
PubSubClient clientMqtt(espClient);
long lastMsgTime = 0;
char msg[50];
int value = 0;

BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
BME280::PresUnit presUnit(BME280::PresUnit_Pa);
float temp(NAN), hum(NAN), pres(NAN);

//////////////////////////////////////////////////////////////////
void readBME280()
{
  bme.read(pres, temp, hum, tempUnit, presUnit);
  pres = pres / 100;
}
//////////////////////////////////////////////////////////////////
void setup_wifi() {
  int interv = 0;
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  #ifdef ESP32
    WiFi.begin(ssid, password);
    WiFi.setHostname(MQTT_REGISTRATION_NAME);
  #else
    WiFi.hostname(MQTT_REGISTRATION_NAME);
    WiFi.begin(ssid, password);
  #endif

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    interv++;
    if (interv > 40) break;
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
//////////////////////////////////////////////////////////////////
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
//////////////////////////////////////////////////////////////////
void reconnectMqtt() {
  int interv = 0;
  // Loop until we're reconnected
  while (!clientMqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (clientMqtt.connect(MQTT_REGISTRATION_NAME)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(clientMqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    interv++;
    if (interv > 6) break;
  }
}
//////////////////////////////////////////////////////////////////
void publishToMqtt(){
    // Convert the value to a char array
    char tempString[8];
    dtostrf(temp, 1, 2, tempString);
    Serial.print("Temperature: ");
    Serial.println(tempString);
    strcpy (mqttFullName, mqttName);
    clientMqtt.publish(strcat(mqttFullName,"/temperature"), tempString);
    
    // Convert the value to a char array
    char humString[8];
    dtostrf(hum, 1, 2, humString);
    Serial.print("Humidity: ");
    Serial.println(humString);
    strcpy (mqttFullName, mqttName);
    clientMqtt.publish(strcat(mqttFullName,"/humidity"), humString);

    // Convert the value to a char array
    char presString[8];
    dtostrf(pres, 1, 2, presString);
    Serial.print("Pressure: ");
    Serial.println(presString);
    strcpy (mqttFullName, mqttName);
    clientMqtt.publish(strcat(mqttFullName,"/pressure"), presString);
    delay(5000);
}
//////////////////////////////////////////////////////////////////
void startI2c(){
  int interv = 0;
  Wire.begin(0, 2);
  while(!bme.begin())  { 
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
    interv++;
    if (interv > 6) break;
  }
 }
//////////////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(SERIAL_BAUD);
  while(!Serial) {} // Wait
  #ifdef ESP32
    Serial.println("ESP32 used");
  #else
    Serial.println("ESP8266 used");
  #endif
  startI2c();
  setup_wifi();
  clientMqtt.setServer(mqtt_server, 1883);
  clientMqtt.setCallback(callback);
}
//////////////////////////////////////////////////////////////////
void loop()
{
  unsigned long now = millis();
  if (!clientMqtt.connected()) {
    reconnectMqtt();
  }
  clientMqtt.loop();
  readBME280();
  lastMsgTime = now;
  publishToMqtt();
  Serial.println("I'm awake, but I'm going into deep sleep");
  #ifdef ESP32
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
  #else
    ESP.deepSleep(TIME_TO_SLEEP * uS_TO_S_FACTOR); 
  #endif
}
