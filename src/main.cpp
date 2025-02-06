#include <OneWire.h>
#include <FastBot2.h>
#include <math.h>
#include <GyverHTTP.h>
#include <Wire.h>
#include <CD74HC4067.h>
#include <TimeLib.h>
#include <ArduinoMqttClient.h>
#include <Arduino_ConnectionHandler.h>
#include <Wire.h>
#include <GSON.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>


// Определения и инициализация
#define BOT_TOKEN "7939415831:AAFTq2tiWJpiH3KgoT09Ymhw80G2tbke5Hs"
#define CHAT_ID "860718018"
#define ONE_WIRE_BUS 2
#define TDS_CHANNEL 4
#define PH_CHANNEL 11
#define TURBIDITY_CHANNEL 15


const char* ssid = "molvinskayaaa";
const char* password = "yaroslav2016";
const String apiKey = "9ce0bce380394587a11102931253101"; //API ключ для OpenWeatherMap до 14.01


FastBot2 bot;
OneWire oneWire(ONE_WIRE_BUS);
CD74HC4067 mux(16, 5, 4, 0);
ESP8266WebServer server(80);


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


enum BotState {
    NORMAL,
    WAITING_FOR_CITY
};


BotState currentState = NORMAL; // Инициализация в нормальное состояние
String city = ""; // Служебная переменная для хранения города


String userName = "";
String wateringTime = ""; // Хранит время полива в формате "HH:MM"
bool wateringScheduled = false;
time_t nextWateringTime = 0;
int wateringDelay = 0;
unsigned long previousMillis = 0; // Сохраняет время последнего запроса
const long interval = 600000; // Интервал между запросами (600000 мс = 10 минут)


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


String getWeatherDescription(String englishDescription) {
    // Функция для перевода описаний погоды
    if (englishDescription == "Partly cloudy") return "🌤 Частично облачно";
    if (englishDescription == "Sunny") return "☀️ Солнечно";
    if (englishDescription == "Overcast") return "☁️ Пасмурно";
    if (englishDescription == "Rain") return "☔ Дождь";
    if (englishDescription == "Clear") return "🌞 Ясно";
    if (englishDescription == "Cloudy") return "☁️ Облачно";
    if (englishDescription == "Mist") return "🌫️ Мгла";
    // Добавьте другие переводы по мере необходимости
    return englishDescription; // Возвращаем оригинальное описание, если перевод не найден
}

void greetUser() {
    fb::Message msg("🌼 Здравствуйте! Я ваш помощник по уходу за растениями! Как я могу у вам обращаться?", CHAT_ID);
    bot.sendMessage(msg);
}

void getWeather() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;


        String url = "http://api.weatherapi.com/v1/current.json?key=" + apiKey + "&q=" + city;
        Serial.print("Requesting weather data from: ");
        Serial.println(url);


        http.begin(client, url);
        int httpCode = http.GET();


        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                Serial.println("Weather data received:");
                Serial.println(payload);


                // Десериализация JSON
                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, payload);
                if (!error) {
                    String description = doc["current"]["condition"]["text"].as<String>();
                    // Использование функции для перевода описания погоды
                    description = getWeatherDescription(description);
                    
                    float temperature = doc["current"]["temp_c"];
                    String weatherMessage = "☁️Погода в " + city + ": " + description + ", Температура: " + String(temperature) + "°C";
                    bot.sendMessage(fb::Message(weatherMessage, CHAT_ID));
                } else {
                    Serial.println("Failed to parse JSON!"); // Ошибка десериализации
                }
            }
        } else {
            Serial.printf("Error on HTTP request: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    } else {
        Serial.println("WiFi not connected");
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
            fb::InlineMenu menu("Полив ; Добавление субстратов ; Погода", "watering;add_substrate;/weather");
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
        } else if (queryData == "/advice") {
            // Обработка кнопки "Советы"
            fb::Message msg("Выберите категорию совета:", u.query().from().id());
            fb::InlineMenu menu("Температура ; Влажность ; Почва ; Удобрение ; Корни",
                                "temperature;humidity;soil;fertilizer;roots");
            msg.setInlineMenu(menu);
            bot.sendMessage(msg);
        } else if (queryData == "temperature") {
            // Советы по температуре
            String adviceMessage = "🌡️ Температура:\n"
                            "Оптимальная температура для большинства комнатных растений составляет 20-25°C. "
                            "Избегайте резких перепадов температуры.";
            bot.sendMessage(fb::Message(adviceMessage, u.query().from().id()));


            // Запрос о полезности совета
            fb::Message feedbackMsg("Был ли этот совет полезен?", u.query().from().id());
            fb::InlineMenu feedbackMenu("👍 ; 👎", "helpful;not_helpful");
            feedbackMsg.setInlineMenu(feedbackMenu);
            bot.sendMessage(feedbackMsg);
        } else if (queryData == "humidity") {
            // Советы по влажности
            String adviceMessage = "💧 Влажность:\n"
                                "Комнатные растения предпочитают влажность на уровне 50-70%. "
                                "Проверьте уровень влажности и используйте увлажнитель, если необходимо.";
            bot.sendMessage(fb::Message(adviceMessage, u.query().from().id()));


            // Запрос о полезности совета
            fb::Message feedbackMsg("Был ли этот совет полезен?", u.query().from().id());
            fb::InlineMenu feedbackMenu("👍 ; 👎", "helpful;not_helpful");
            feedbackMsg.setInlineMenu(feedbackMenu);
            bot.sendMessage(feedbackMsg);
        } else if (queryData == "soil") {
            // Советы по почве
            String adviceMessage = "🌱 Почва:\n"
                                "Используйте легкую, хорошо дренированную почву для комнатных растений. "
                                "Регулярно проверяйте уровень pH для ваших растений.";
            bot.sendMessage(fb::Message(adviceMessage, u.query().from().id()));


            // Запрос о полезности совета
            fb::Message feedbackMsg("Был ли этот совет полезен?", u.query().from().id());
            fb::InlineMenu feedbackMenu("👍 ; 👎", "helpful;not_helpful");
            feedbackMsg.setInlineMenu(feedbackMenu);
            bot.sendMessage(feedbackMsg);
        } else if (queryData == "fertilizer") {
            // Советы по удобрениям
            String adviceMessage = "🌼 Удобрение:\n"
                                "Подкармливайте растения каждые 4-6 недель в течение вегетационного периода. "
                                "Используйте удобрения, сбалансированные по основным микроэлементам.";
            bot.sendMessage(fb::Message(adviceMessage, u.query().from().id()));


            // Запрос о полезности совета
            fb::Message feedbackMsg("Был ли этот совет полезен?", u.query().from().id());
            fb::InlineMenu feedbackMenu("👍 ; 👎", "helpful;not_helpful");
            feedbackMsg.setInlineMenu(feedbackMenu);
            bot.sendMessage(feedbackMsg);
        } else if (queryData == "roots") {
            // Советы по корням
            String adviceMessage = "🪴 Корни:\n"
                                "Следите за здоровьем корней вашего растения. "
                                "Если корни перегружены, пересаживайте растение в новую почву.";
            bot.sendMessage(fb::Message(adviceMessage, u.query().from().id()));


            // Запрос о полезности совета
            fb::Message feedbackMsg("Был ли этот совет полезен?", u.query().from().id());
            fb::InlineMenu feedbackMenu("👍 ; 👎", "helpful;not_helpful");
            feedbackMsg.setInlineMenu(feedbackMenu);
            bot.sendMessage(feedbackMsg);
        } else if (queryData == "helpful") {
            bot.sendMessage(fb::Message("Спасибо за ваш отзыв! Рад, что совет был полезен.", u.query().from().id()));
        } else if (queryData == "not_helpful") {
            bot.sendMessage(fb::Message("Спасибо, что сообщили. Мы постоянно улучшаем наши советы!", u.query().from().id()));
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
        } else if (queryData == "/weather") {
            // Проверяем, установлен ли город
            if (city == "") {
                bot.sendMessage(fb::Message("Пожалуйста, введите ваш город:", u.query().from().id()));
                currentState = WAITING_FOR_CITY; // Устанавливаем состояние ожидания города
            } else {
                // Запрос погоды
                getWeather();
                // После успешного запроса погоды предлагаем изменить город
                fb::Message changeCityMsg("Хотите сменить город?", u.query().from().id());
                fb::InlineMenu changeCityMenu("Сменить город ; Нет", "change_city;no_change");
                changeCityMsg.setInlineMenu(changeCityMenu);
                bot.sendMessage(changeCityMsg);
            }
        } else if (queryData == "change_city") {
            bot.sendMessage(fb::Message("Пожалуйста, введите новый город:", u.query().from().id()));
            currentState = WAITING_FOR_CITY; // Снова устанавливаем состояние ожидания города
        } else if (queryData == "no_change") {
            bot.sendMessage(fb::Message("Хорошо, оставим текущий город: " + city, u.query().from().id()));
        }
        } else {
            String userMessage = String(u.message().text().c_str());
        // Проверка, если состояние ожидания города
            if (currentState == WAITING_FOR_CITY) {
                city = userMessage; // Сохраняем город, введенный пользователем
                getWeather(); // Сразу запрашиваем погоду после установки города
                currentState = NORMAL; // Возвращаемся в нормальное состояние
                return;
            }
        // Обработка обычных сообщений
        Serial.println("NEW MESSAGE");
        Serial.println(u.message().from().username());
        Serial.println(u.message().text());
        //String userMessage = String(u.message().text().c_str());


        // Проверка, если состояние ожидания города
        if (currentState == WAITING_FOR_CITY) {
            city = userMessage; // Сохраняем город, введенный пользователем
            bot.sendMessage(fb::Message("Город установлен: " + city + ". Теперь вы можете использовать команду /weather.", u.message().chat().id()));
            currentState = NORMAL; // Возвращаемся в нормальное состояние
            return;
        }


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
            fb::InlineMenu menu("Показания ; Уведомления ; Советы ; Погода ; Статистика ; Обновить ; Помощь",
                                "/readings;/notifications;/advice;/weather;/stats;/refresh;/help");
            msg.setInlineMenu(menu);
            bot.sendMessage(msg);
        } else {
            bot.sendMessage(fb::Message(userName + ", " + userMessage, u.message().chat().id()));
        }
    }
}


void handleRoot() {
    String muxResults = getMuxData(); // Получаем результаты с мультиплексора
    String html = "<html><head>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f4f4f4; }";
    html += "h1 { color: #4CAF50; }"; // Цвет заголовка
    html += "h2 { color: #2196F3; }"; // Цвет второго заголовка
    html += ".container { max-width: 800px; margin: auto; padding: 20px; background: white; border-radius: 10px; box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1); }";
    html += ".thermometer { width: 50px; height: 300px; background: #e0e0e0; border-radius: 25px; position: relative; margin: 20px auto; }";
    html += ".thermometer-fill { position: absolute; bottom: 0; width: 100%; border-radius: 25px; }";
    html += ".temperature-label { position: absolute; left: 1000px; font-size: 24px; color: #4CAF50; margin-top: -60px; }";
    html += ".results { white-space: pre-wrap; margin-top: 850px; }"; // Пробелы и перенос строк сохраняются
    html += "table { width: 100%; border-collapse: collapse; margin-top: 20px; }";
    html += "th, td { border: 1px solid #ccc; padding: 10px; text-align: left; }";
    html += "th { background-color: #f2f2f2; }";
    html += "</style></head><body>";
    html += "<div class='container'>";




    // Вычисление высоты для термометра
    float temperatureHeight = constrain(ds18b20.temperature * 10, 0, 300);
    // Определение цвета заполнения термометра в зависимости от температуры
    String fillColor;
    if (ds18b20.temperature <= 27) {
        fillColor = "green"; // до 25°C - зеленый
    } else if (ds18b20.temperature > 27 && ds18b20.temperature <= 35) {
        fillColor = "yellow"; // от 25°C до 35°C - желтый
    } else {
        fillColor = "red"; // свыше 35°C - красный
    }


    html += "<h1>Данные сенсоров</h1>";
    html += "<div class='thermometer'>";
    html += "<div class='thermometer-fill' style='height: " + String(temperatureHeight) + "px; background-color: " + fillColor + ";'></div>"; // Изменяем цвет заполнения
    html += "</div>";
    html += "<div class='temperature-label'>" + String(ds18b20.temperature) + " °C</div>";

    // Добавление таблицы для других показателей
    html += "<h2>Качество воды</h2>";
    html += "<table>";
    html += "<tr><th>Показатель</th><th>Значение</th></tr>";




    // Заполнение таблицы (например, вы можете извлекать значения из функции getMuxData)
    String results = muxResults; // Ваши результаты из getMuxData
    String lines[3];
    int lineCount = 0;




    // Разбиваем полученные результаты на строки для таблицы
    int startIndex = 0;
    while (startIndex < results.length()) {
        int endIndex = results.indexOf("\n", startIndex);
        if (endIndex == -1) endIndex = results.length();
        lines[lineCount] = results.substring(startIndex, endIndex);
        startIndex = endIndex + 1;
        lineCount++;
    }
    // Обработка каждой строки для отображения в таблице
    for (int i = 0; i < lineCount; i++) {
        String label = lines[i].substring(0, lines[i].indexOf(":")); // Название показателя
        String value = lines[i].substring(lines[i].indexOf(":") + 1); // Значение показателя
        html += "<tr><td>" + label + "</td><td>" + value + "</td></tr>";
    }


    html += "</table>"; // Закрытие таблицы
    html += "</div>"; // Закрываем контейнер
    html += "</body></html>";
    server.send(200, "text/html; charset=UTF-8", html);
}



void setup() {
    Serial.begin(9600);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println(WiFi.localIP());


    ds18b20.setup();
    bot.attachUpdate(updateh);
    bot.setToken(BOT_TOKEN);
    bot.setPollMode(fb::Poll::Long, 20000);
    greetUser();
    server.on("/", handleRoot); // Установка маршрута на корень
    server.begin();


    getWeather(); // Первоначальный запрос погоды
}




void loop() {
    server.handleClient();
    if (!ds18b20.busy) {
        ds18b20.start();
    }
    checkBusyDS18B20();


    if (!ds18b20.busy) {
        getDataDS18B20();
    }


    // Периодический запрос данных о погоде
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis; // обновляем время последнего запроса
        getWeather(); // вызываем запрос к API с погодой
    }


    checkWateringTime();
    delay(3000);
    bot.tick();
}
