#include <Arduino.h>
#include "lionbit.h"
#include "DHT.h"
#include <WiFi.h>
#include <WiFiClient.h>

#define DHTPIN D4

#define DHTTYPE DHT11

const char *SSID = "TOZED_4G";
const char *PSWD = "653D44A7";

const char *HOST = "api.thingspeak.com"; //

const char *PRIVATEKEY = "P4Q63MKDI1T7DBT2"; // Write API Key from thingspeak {Feel free to use this API}

volatile double updateTime = 0;
volatile double ledTimeOut = 0;
/* Error List */
const char *ERRORCODES[] = {

    "WF_101",
    "SF_102",
    "IoTP_103"};

/* In order to return list of sensors' data */
typedef struct SENSORS
{
  int temperature;
  int humidity;
  int soilMositureLevel;
  bool err;

} sensor_t;

/* CallBack function */
typedef int (*event_t)(int, int, int); /* This callbackf function will use to send data to Thingspeak Sever */
typedef bool (*mainEvents_t)(char *, char *);

/* Declaring prtotype */
int sendThingSpeakSever(int tmp, int hum, int soilHum);
bool wifiConnection(char *ssid, char *pswd);
sensor_t readingSensors();
void systemInit();

mainEvents_t WiFiConnection = &wifiConnection;
DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  dht.begin();
  systemInit();
}

void loop()
{
  /* sending collected data to ThingSpeak Sever */
  if (millis() - updateTime > 5000UL)
  {
    event_t thingsSpeak = &sendThingSpeakSever;

    sensor_t sesnorsValue = readingSensors();

    updateTime = millis();
    if (sesnorsValue.err && (WiFi.status() == WL_CONNECTED))
    {
      
      thingsSpeak(sesnorsValue.temperature, sesnorsValue.humidity, sesnorsValue.soilMositureLevel);
    }
  }

  if (millis() - ledTimeOut > 150 && ((WiFi.status() == WL_CONNECTED)))
  {
    ledTimeOut = millis();

    digitalWrite(LED_BUILTIN, (1 - digitalRead(LED_BUILTIN)));
  }
}

void systemInit()
{

  if (WiFiConnection((char *)SSID, (char *)PSWD))
  {
    /* Event Body */
    Serial.printf("WiFi Sucesse \n");
  }

  else
  {
    Serial.printf("Error Code : %s \n", ERRORCODES[0]);
  }
}

bool wifiConnection(char *ssid, char *pswd)
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PSWD);
  if (WiFi.status() != WL_CONNECTED)
  {
    return false;
  }
  return true;
}

sensor_t readingSensors()
{
  sensor_t sensorData;
  int h = dht.readHumidity();

  int t = dht.readTemperature();
 
  if (isnan(h) || isnan(t))
  {
    Serial.printf("Error Code : %s \n", ERRORCODES[1]);
    sensorData.err = false;
  }
  else
  {
    int soilSesnor = 0;

    for (int i = 0; i < 50; i++)
    {
      soilSesnor += analogRead(A2);
    }

    sensorData.err = true;
    sensorData.humidity = h;
    sensorData.temperature = t;
    sensorData.soilMositureLevel = soilSesnor / 50;
  }

  return (sensorData);
}

int sendThingSpeakSever(int tmp, int hum, int soilHum)
{
  WiFiClient client; // Creating Wifi clie
  const int httpPort = 80;
  client.connect((char *)HOST, httpPort);

  char url[150];
  sprintf(url, "/update?key=%s&field1=%d&field2=%d&field3=%d", PRIVATEKEY, (int)tmp, (int)hum, (int)soilHum);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + HOST + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long int timeout = millis() + 5000;

  while (client.available() == 0)
  {
    if (timeout - millis() < 0)
    {
      Serial.println(">>> Client Timeout !");
      client.stop();
      Serial.printf("Error Code : %s \n", ERRORCODES[2]);
    }
  }

  return 0;
}
