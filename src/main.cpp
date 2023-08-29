#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include "camera.h"

#include "config.h"

TaskHandle_t serialToNetHandle;
TaskHandle_t netToSerialHandle;
WiFiMulti wifiMulti;
WiFiServer server(23);
WiFiClient client;

void serialConnect();
void wifiConnect();
void initServer();
void clientConnect();
void serialToNetTaskFunc(void *pvParameters);
void netToSerialTaskFunc(void *pvParameters);
void printMessageToMarlinDisplay(char *message);
void reconnect();


void setup()
{

  // put your setup code here, to run once:
  serialConnect();
  wifiConnect();
  //carmeraInit();
  clientConnect();
  xTaskCreatePinnedToCore(serialToNetTaskFunc, "serailToNet", 10000, NULL, configMAX_PRIORITIES - 1, &serialToNetHandle, 0);
  xTaskCreatePinnedToCore(netToSerialTaskFunc, "netToSerial", 10000, NULL, configMAX_PRIORITIES - 1, &netToSerialHandle, 0);
  xTaskCreatePinnedToCore(carmeraInit, "carmeraInit", 10000, NULL, configMAX_PRIORITIES - 1, NULL, 1);
}

void loop()
{
  // put your main code here, to run repeatedly:
  // if (client && client.connected() && Serial.available() ){
  // size_t len = Serial.available();
  // uint8_t sbut[len];
  // Serial.reaqBytes(sbuf,len);

  // client.wirte(sbuf,len);

  // }
  delay(1000);
}

void serialConnect()
{
  Serial.begin(115200);
  while (!Serial)
  {
    delay(100);
  }
  printMessageToMarlinDisplay("ESP32 connected");
}

void wifiConnect()
{
  WiFi.begin(ssid, password);
  printMessageToMarlinDisplay("Wifi connecting.");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
  // Serial.write("M117 ");
  // Serial.write(WiFi.localIP());
  // Serial.write("\n");v
  printMessageToMarlinDisplay("Wifi connected");
  initServer();
}

void initServer()
{
  server.begin();
  server.setNoDelay(true);
  printMessageToMarlinDisplay("Server Ready");
}

void clientConnect()
{
  for (;;)
  {
    if (server.hasClient())
    {
      if (!client || !client.connected())
      {
        client = server.available();
        printMessageToMarlinDisplay("client connected");
        break;
      }
    }
    delay(20);
  }
}

void serialToNetTaskFunc(void *pvParameters)
{
  uint8_t sbuf[BUF_SIZE];
  size_t len;
  for (;;)
  {
    if (client && client.connected())
    {
      while (Serial.available())
      {
        len = Serial.available();
        if (len > BUF_SIZE)
        {
          len = BUF_SIZE;
        }
        Serial.readBytes(sbuf, len);
        client.write(sbuf, len);
        
      }
    }
    else{
      Serial.flush();
    }
    delay(20);
  }
}

void netToSerialTaskFunc(void *pvParameters)
{
  uint8_t sbuf[BUF_SIZE];
  size_t len;
  for (;;)
  {
    if (client && client.connected() && (WiFi.status() == WL_CONNECTED))
    {
      while (client.available())
      {
        len = client.available();
        if (len > BUF_SIZE)
        {
          len = BUF_SIZE;
        }
        client.readBytes(sbuf, len);
        Serial.write(sbuf,len);
      }
    }
    else
    {
      printMessageToMarlinDisplay("disconnected");
      reconnect();
    }

    delay(20);
  }
}

void printMessageToMarlinDisplay(char *message)
{
  Serial.write("M117 ");
  Serial.write(message);
  Serial.write("\n");
}

void reconnect()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    printMessageToMarlinDisplay("Wifi reconnect");
    WiFi.reconnect();

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(100);
    }
    printMessageToMarlinDisplay("Wifi connected");
  }
  if (client)
  {
    if (!client.connected())
    {
      client.stop();
      client = NULL;
      clientConnect();
    }
  }
  else{
    clientConnect();
  }
}