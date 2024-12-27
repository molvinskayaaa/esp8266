#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <FastBot2.h>
#define "7047784610:AAFFKiqiwSQRTBwoflOyeUCF-xC9N1qOWbI"

FastBot2 bot;

struct WiFiConfig {
  const char* ssid;
  const char* password;
};

WiFiConfig wifiConfig = {
  "molvinskayaaa",
  "yaroslav2016"
};

struct DS18B20Sensor {
    byte addr[8];
    float temperature;
    bool started;
    bool busy;
    byte data[9];
};

DS18B20Sensor ds18b20 = {
    {0},
    0,
    false,
    false,
    {0}
};


#define ONE_WIRE_BUS 2

int Bot_mtbs = 1000; 
long Bot_lasttime;

OneWire oneWire(ONE_WIRE_BUS);

void printAddress(byte addr[8]) {
  for (byte i = 0; i < 8; i++) {
    if (addr[i] < 16) Serial.print("0");
    Serial.print(addr[i], HEX);
    if (i < 7) {
      Serial.print(":");
    }
  }
}

void setupDS18b20() {
  oneWire.reset_search();              
  if (!oneWire.search(ds18b20.addr)) {  
    Serial.println("No DS18B20 sensor found");
    return;
  }
  Serial.print("Found device with address: ");
  printAddress(ds18b20.addr);
  Serial.println();

  if (OneWire::crc8(ds18b20.addr, 7) != ds18b20.addr[7] || ds18b20.addr[0] != 0x28) {
    Serial.println("Sensor is not a DS18B20");
    return;
  }
  Serial.println("Sensor is a DS18B20");
}

void startMeasurementDS18b20() {
  if (!ds18b20.started) {
    oneWire.reset();
    oneWire.select(ds18b20.addr);
    oneWire.write(0x44); 
    ds18b20.started = true;
    ds18b20.busy = true;
  }
}

void newMsg(FB_msg& msg) {
  Serial.print(msg.chatID);     // ID чата 
  Serial.print(", ");
  Serial.print(msg.username);   // логин
  Serial.print(", ");
  Serial.println(msg.text);     // текст
}

void setup() {
  Serial.begin(9600);
  WiFi.begin(wifiConfig.ssid, wifiConfig.password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  setupDS18b20();
  bot.attach(newMsg);
}


void loop() {
  startMeasurementDS18b20();
  

  // Получаем обновления от Telegram бота
  // if (millis() > Bot_lasttime + Bot_mtbs) {
  //     bot.getUpdates(); // Проверьте правильность вызова
  //     Serial.println("Обновления получены");
  //     Bot_EchoMessages();
  //     Bot_lasttime = millis();
  // }
}
