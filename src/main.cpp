#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <FastBot2.h>
#include <math.h>
#include <GyverHTTP.h>
#include <Wire.h>
#include <CD74HC4067.h>
#include <TimeLib.h>
#include <ArduinoMqttClient.h>
#include <Arduino_ConnectionHandler.h>


// Определения и инициализация
#define BOT_TOKEN "token"
#define CHAT_ID "id"
#define ONE_WIRE_BUS 2
#define TDS_CHANNEL 4
#define PH_CHANNEL 11
#define TURBIDITY_CHANNEL 15


FastBot2 bot;
OneWire oneWire(ONE_WIRE_BUS);
CD74HC4067 mux(16, 5, 4, 0);


struct DS18B20Sensor {
    byte addr[8];
    float temperature;
    bool started;
    bool busy;
    byte data[9];
    unsigned long startTime;
};


DS18B20Sensor ds18b20 = {
    {0},
    0,
    false,
    false,
    {0},
    0
};


struct WiFiConfig {
    const char* ssid;
    const char* password;
};


WiFiConfig wifiConfig = {
    "molvinskayaaa",
    "yaroslav2016"
};


String userName = "";
String wateringTime = ""; // Хранит время полива в формате "HH:MM"
bool wateringScheduled = false;
time_t nextWateringTime = 0;
int wateringDelay = 0;


void printAddress(byte addr[8]) {
    for (byte i = 0; i < 8; i++) {
        if (addr[i] < 16) Serial.print("0"); // выводим ведущий ноль
        Serial.print(addr[i], HEX); // выводим адрес в шестнадцатеричном формате
        if (i < 7) {
            Serial.print(":"); // разделяем адрес двоеточием
        }
    }
}


void setupDS18b20() {
    oneWire.reset_search(); // сброс поиска
    if (!oneWire.search(ds18b20.addr)) {
        Serial.println("No DS18B20 sensor found"); // не найден датчик
        return; // выход из функции
    }
    Serial.print("Found device with address: ");
    printAddress(ds18b20.addr); // вывод адреса найденного устройства
    Serial.println();
    if (OneWire::crc8(ds18b20.addr, 7) != ds18b20.addr[7] || ds18b20.addr[0] != 0x28) {
        Serial.println("Sensor is not a DS18B20"); // проверка типа датчика
        return; // выход из функции
    }
    Serial.println("Sensor is a DS18B20"); // подтверждение типа датчика
}


void startMeasurementDS18b20() {
    if (!ds18b20.started) { // если измерение не начато
        oneWire.reset(); // сброс OneWire
        oneWire.select(ds18b20.addr); // выбор устройства
        oneWire.write(0x44); // запуск измерения
        ds18b20.started = true; // установка состояния начато
        ds18b20.busy = true; // установка состояния занято
        ds18b20.startTime = millis(); // запоминаем время начала
        Serial.println("Measurement started"); // вывод сообщения о начале измерения
    }
}




void checkBusyDS18B20() {
    if (ds18b20.started && ds18b20.busy) { // если измерение начато и занято
        if (oneWire.read_bit()) { // проверяем завершение измерения
            Serial.println("Measurement completed"); // вывод сообщения о завершении
            ds18b20.busy = false; // сброс состояния занятости
        } else if (millis() - ds18b20.startTime >= 750) { // таймаут 750 мс
            ds18b20.temperature = -127; // неверное значение температуры
            Serial.println("Timeout: Measurement assumed completed"); // сообщение о таймауте
            ds18b20.busy = false; // сброс состояния
        }
    }
}


void getDataDS18B20() {
    if (ds18b20.started && !ds18b20.busy) { // если измерение начато и не занято
        oneWire.reset(); // сброс OneWire
        oneWire.select(ds18b20.addr); // выбор устройства
        oneWire.write(0xBE); // запрос на чтение данных
        for (int i = 0; i < 9; i++) {
            ds18b20.data[i] = oneWire.read(); // считываем данные
        }
        if (OneWire::crc8(ds18b20.data, 8) != ds18b20.data[8]) {
            Serial.println("CRC check failed. Invalid data received."); // проверка CRC
            ds18b20.temperature = -127; // неверное значение температуры
        } else {
            int16_t rawTemperature = (ds18b20.data[1] << 8) | ds18b20.data[0]; // преобразование данных
            ds18b20.temperature = (float)rawTemperature / 16.0; // вычисление температуры
        }
        Serial.print("Temperature: "); // вывод температуры
        Serial.print(ds18b20.temperature);
        Serial.println(" C");
        // Проверка температуры и отправка уведомлений
        if (ds18b20.temperature > 30.0) {
            bot.sendMessage(fb::Message("Внимание! Температура зашкаливает: " + String(ds18b20.temperature) + " C", CHAT_ID));
        } else if (ds18b20.temperature < 10.0) {
            bot.sendMessage(fb::Message("Внимание! Температура слишком низкая для растения: " + String(ds18b20.temperature) + " C", CHAT_ID));
        }
        ds18b20.started = false; // сброс для следующего измерения
    }
}


String getMuxData() {
    String results = ""; // строка для хранения результатов
    int channels[] = {4, 11, 15}; // каналы для считывания
    String labels[] = {"Содержание солей (TDS)", "Кислотность (pH)", "Мутность (NTU)"}; // Названия показателей


    for (int i = 0; i < sizeof(channels) / sizeof(channels[0]); i++) {
        int channel = channels[i];
        mux.channel(channel); // выбираем канал
        int value = analogRead(A0); // считываем значение




        // Обработка значений в зависимости от канала
        float calibratedValue;
        String alert = "";
        if (channel == 4) {
            int maxTDS = 500; // Максимально допустимый уровень TDS (ppm)
            calibratedValue = (value / 1023.0) * maxTDS; // Калибровка для TDS
            // Проверка для TDS
            if (calibratedValue > maxTDS) {
                alert = "Предупреждение: содержание солей превышает " + String(maxTDS) + " ppm.";
            }
        } else if (channel == 11) {
            float minPH = 7.0;
            float maxPH = 10.0;
            calibratedValue = minPH + ((value / 1023.0) * (maxPH - minPH)); // Калибровка для pH
            // Проверка для pH
            if (calibratedValue > maxPH) {
                alert = "Предупреждение: кислотность превышает " + String(maxPH) + ".";
            }
        } else if (channel == 15) {
            int maxTurbidity = 40; // Максимально допустимый уровень мутности
            calibratedValue = (value / 1023.0) * maxTurbidity; // Калибровка для мутности
            // Проверка для мутности
            if (calibratedValue > maxTurbidity) {
                alert = "Предупреждение: мутность превышает " + String(maxTurbidity) + " NTU.";
            }
        }
        // Добавляем результат в строку с названием показателя
        results += labels[i] + ": " + String(calibratedValue) + "\n"; // Используем откалиброванные значения
        if (alert != "") {
            results += alert + "\n"; // Добавляем предупреждение, если есть
        }
    }
    return results; // возвращаем результаты
}



void checkWateringTime() {
    if (nextWateringTime > 0 && now() >= nextWateringTime) {
        // Отправка уведомления
        bot.sendMessage(fb::Message("Пора полить ваш цветок! Время для полива.", CHAT_ID));
        // Сбрасываем время полива, чтобы уведомление не отправлялось повторно
        nextWateringTime = 0;
        wateringDelay = 0; // Сброс переменной задержки
    }
}


void updateh(fb::Update& u) {
    if (u.isQuery()) {
        Serial.println("NEW QUERY");
        Serial.println(u.query().data());
        bot.answerCallbackQuery(u.query().id(), "query answered");
        String queryData = String(u.query().data().c_str());
        if (queryData == "/readings") {
            // Обработка получения показаний
            if (ds18b20.busy) {
                bot.sendMessage(fb::Message("Измерение в процессе. Пожалуйста, подождите.", u.query().from().id()));
            } else {
                startMeasurementDS18b20();
                delay(1000);
                getDataDS18B20();
                String muxResults = getMuxData();
                String results = "Текущая температура: " + String(ds18b20.temperature) + " C\n" + muxResults;
                bot.sendMessage(fb::Message(results, u.query().from().id()));
            }
        } else if (queryData == "/notifications") {
            // Обработка кнопки "Уведомления"
            fb::Message msg("Выберите уведомление:", u.query().from().id());
            fb::InlineMenu menu("Полив ; Добавление субстратов", "watering;add_substrate");
            msg.setInlineMenu(menu);
            bot.sendMessage(msg);
        } else if (queryData == "watering") {
            // Обработка кнопки "Полив"
            fb::Message msg("Через сколько вам удобно полить цветок?", u.query().from().id());
            fb::InlineMenu menu("5 мин ; 10 мин ; 15 мин ; 30 мин ; 45 мин ; 1 час ; 2 часа",
                                "watering_5;watering_10;watering_15;watering_30;watering_45;watering_60;watering_120");
            msg.setInlineMenu(menu);
            bot.sendMessage(msg);
        } else if (queryData.startsWith("watering_")) {
            // Обработка выбора времени полива
            if (queryData == "watering_5") wateringDelay = 5 * 60; // 5 минут
            else if (queryData == "watering_10") wateringDelay = 10 * 60; // 10 минут
            else if (queryData == "watering_15") wateringDelay = 15 * 60; // 15 минут
            else if (queryData == "watering_30") wateringDelay = 30 * 60; // 30 минут
            else if (queryData == "watering_45") wateringDelay = 45 * 60; // 45 минут
            else if (queryData == "watering_60") wateringDelay = 1 * 60 * 60; // 1 час
            else if (queryData == "watering_120") wateringDelay = 2 * 60 * 60; // 2 часа
           
            time_t nowTime = now();
            nextWateringTime = nowTime + wateringDelay; // Установка следующего времени полива
            bot.sendMessage(fb::Message("Время полива установлено через " + String(wateringDelay / 60) + " минут(ы).", u.query().from().id()));
        } else if (queryData == "add_substrate") {
            // Обработка кнопки "Добавление субстратов"
            bot.sendMessage(fb::Message("Уведомление о добавлении субстратов установлено.", u.query().from().id()));
        } else if (queryData == "/help") {
            String helpMessage = "🌿 Инструкция по использованию бота:\n\n"
                                "1. 📊 Показания - получение текущих данных:\n"
                                "   • Температура воздуха\n"
                                "   • Кислотность (pH)\n"
                                "   • Мутность воды\n"
                                "   • Содержание солей (TDS)\n\n"
                                "2. 🔔 Уведомления:\n"
                                "   • Настройка времени полива\n"
                                "   • Напоминания о добавлении субстратов\n\n"
                                "3. 💡 Советы - рекомендации по уходу за растением:\n"
                                "   • Оптимальные условия\n"
                                "   • Частота полива\n"
                                "   • Подкормка\n\n"
                                "4. ⚙️ Калибровка - настройка датчиков для точных измерений\n\n"
                                "5. 📈 Статистика - история показаний и графики\n\n"
                                "6. 🔄 Обновить - получение свежих данных\n\n"
                                "❗️ Автоматические уведомления:\n"
                                "• При высокой температуре (>30°C)\n"
                                "• При низкой температуре (<10°C)\n"
                                "• В установленное время полива\n"
                                "• При отклонении показателей от нормы\n\n"
                                "💬 Для использования просто выберите нужную команду в меню.";
            
            bot.sendMessage(fb::Message(helpMessage, u.query().from().id()));
        }
    } else {
        // Обработка обычных сообщений
        Serial.println("NEW MESSAGE");
        Serial.println(u.message().from().username());
        Serial.println(u.message().text());
        String userMessage = String(u.message().text().c_str());
        if (wateringScheduled) {
            // Обработка времени полива
            int hour, minute;
            if (sscanf(userMessage.c_str(), "%d:%d", &hour, &minute) == 2) {
                // Установка времени полива
                time_t nowTime = now();
                tmElements_t timeElements;
                breakTime(nowTime, timeElements);
                timeElements.Hour = hour;
                timeElements.Minute = minute;
                timeElements.Second = 0;
                time_t userTime = makeTime(timeElements);
                if (userTime <= nowTime) {
                    // Если указанное время уже прошло сегодня, устанавливаем на завтра
                    userTime += 24 * 60 * 60;
                }
                nextWateringTime = userTime;
                wateringTime = String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute);
                bot.sendMessage(fb::Message("Время полива установлено на " + wateringTime + ".", u.message().chat().id()));
                wateringScheduled = false;
            } else {
                bot.sendMessage(fb::Message("Неверный формат. Пожалуйста, введите время в формате 08:00.", u.message().chat().id()));
            }
        } else if (userName == "") {
            userName = userMessage;
            String welcomeMessage = "Приятно познакомиться, " + userName + "! Выберите:";
            fb::Message msg(welcomeMessage, u.message().chat().id());
            fb::InlineMenu menu("Показания ; Уведомления ; Советы ; Калибровка ; Статистика ; Обновить ; Помощь",
                                "/readings;/notifications;/advice;/calibrate;/stats;/refresh;/help");
            msg.setInlineMenu(menu);
            bot.sendMessage(msg);
        } else {
            bot.sendMessage(fb::Message(userName + ", " + userMessage, u.message().chat().id()));
        }
    }
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
    bot.attachUpdate(updateh);
    bot.setToken(F(BOT_TOKEN));
    bot.setPollMode(fb::Poll::Long, 20000);
    fb::Message msg("Привет! Как я могу к вам обращаться?", CHAT_ID);
    bot.sendMessage(msg);




}


void loop() {
    if (!ds18b20.busy) {
        startMeasurementDS18b20();
    }
    checkBusyDS18B20();




    if (!ds18b20.busy) {
        getDataDS18B20();
    }
    String muxResults = getMuxData();
    Serial.print(muxResults);
    checkWateringTime();
    delay(3000);
    bot.tick();
}
