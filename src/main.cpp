#include <Arduino.h>
#include "stdio.h"
#include "lionbit.h"
#include "DHT.h"
#include <WiFi.h>
#include <WiFiClient.h>

#define DHTPIN D4

#define DHTTYPE DHT11

const char *SSID = "slt-tigo";
const char *PSWD = "12345678";

const char *HOST = "api.thingspeak.com"; //

const char *PRIVATEKEY = "P4Q63MKDI1T7DBT2"; // Write API Key from thingspeak {Feel free to use this API}


volatile double updateTime = 0; 


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
typedef int (*event_t)(int *, int *, int *); /* This callbackf function will use to send data to Thingspeak Sever */
typedef bool (*mainEvents_t)(char *, char *);

/* Declaring prtotype */
int sendThingSpeakSever(int *tmp, int *hum, int *soilHum);
bool wifiConnection(char *ssid, char *pswd);
sensor_t readingSensors();
void systemInit();

mainEvents_t WiFiConnection = &wifiConnection;
DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  Serial.begin(115200);
  dht.begin();
  systemInit();
}

void loop()
{
  event_t thingsSpeak = &sendThingSpeakSever;

  sensor_t sesnorsValue = readingSensors();

  /* sending collected data to ThingSpeak Sever */
  if (millis() - updateTime > 5000UL)
  {
    updateTime = millis();
    if (sesnorsValue.err && (WiFi.status() != WL_CONNECTED))
    {
      thingsSpeak(&sesnorsValue.temperature, &sesnorsValue.humidity, &sesnorsValue.soilMositureLevel);
    }
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
    sensorData.err = false;
    Serial.printf("Error Code : %s \n", ERRORCODES[1]);
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

int sendThingSpeakSever(int *tmp, int *hum, int *soilHum)
{
  WiFiClient client; // Creating Wifi clie
  const int httpPort = 80;
  client.connect((char *)HOST, httpPort);

  // We now create a URI for the request
  String url = "/update";
  // // url += streamId;
  url += "?key=";
  url += PRIVATEKEY;
  url += "&field1="; // Things peak field1
  url += (int)tmp;   // value that need  put to field1
  url += "&field2="; // Things peak field1
  url += (int) hum;        // value that need  put to field1
  url += "&field3=";
  url += (int) soilHum; // value that need put to field
  // // url += "&field4=";
  // url += ULT;
  // url += "&status=";
  // url += count; // Number of count that transmitted to the thingspeak Server

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
    }
  }

  return 0;
}
