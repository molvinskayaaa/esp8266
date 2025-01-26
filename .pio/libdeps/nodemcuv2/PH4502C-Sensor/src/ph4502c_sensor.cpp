#include <Arduino.h>
#include "ph4502c_sensor.h"

// Определение статических констант
const float PH4502C_Sensor::PH4502C_VOLTAGE = 5.0;              // Напряжение системы
const float PH4502C_Sensor::PH4502C_MID_VOLTAGE = 2.5;           // Среднее напряжение для pH 7.00
const float PH4502C_Sensor::PH4502C_PH_VOLTAGE_PER_PH = 0.18;     // Напряжение на 1 единицу pH
const uint16_t PH4502C_Sensor::_adc_resolution = 1023;           // 10-битное разрешение

// Конструктор класса
PH4502C_Sensor::PH4502C_Sensor(uint8_t ph_level_pin, uint8_t temp_pin, uint16_t reading_count, uint16_t reading_interval) 
    : _ph_level_pin(ph_level_pin), _temp_pin(temp_pin), _reading_count(reading_count), _reading_interval(reading_interval), _calibration(0.0f) {
}

// Метод инициализации
void PH4502C_Sensor::init() {
    pinMode(this->_ph_level_pin, INPUT);
    pinMode(this->_temp_pin, INPUT);
}

// Метод для чтения уровня pH
float PH4502C_Sensor::read_ph_level() {
    float reading = 0.0f;

    for(int i = 0; i < this->_reading_count; i++) {
        reading += analogRead(this->_ph_level_pin);
        delayMicroseconds(this->_reading_interval);
    }

    reading = PH4502C_VOLTAGE / _adc_resolution * reading; // Преобразование аналогового значения в напряжение
    reading /= this->_reading_count;
    reading = this->_calibration + ((PH4502C_MID_VOLTAGE - reading)) / PH4502C_PH_VOLTAGE_PER_PH; // Калибровка

    return reading; // Возвращаем значение pH
}

// Метод для чтения уровня pH (одиночное значение)
float PH4502C_Sensor::read_ph_level_single() {
    float reading = analogRead(this->_ph_level_pin);
    
    reading = PH4502C_VOLTAGE / _adc_resolution * reading; // Преобразование аналогового значения в напряжение
    reading /= this->_reading_count;

    return this->_calibration + ((PH4502C_MID_VOLTAGE - reading)) / PH4502C_PH_VOLTAGE_PER_PH; // Калибровка
}

// Метод для чтения температуры
float PH4502C_Sensor::read_temp() {
    int rawValue = analogRead(this->_temp_pin);  // Чтение сырых данных с пина
    float voltage = rawValue * (PH4502C_VOLTAGE / _adc_resolution);    // Преобразуем в напряжение
    float temperatureC = voltage * 100.0; // Преобразуем напряжение в температуру (100mV/°C)
    
    return temperatureC; // Возвращаем значение температуры
}

// Метод для калибровки
void PH4502C_Sensor::recalibrate(float calibration) {
    this->_calibration = calibration; // Установка нового калибровочного значения
}

// Метод для калибровки на основе стандартного раствора
void PH4502C_Sensor::calibrate_with_standard(float standard_ph, float measured_value) {
    float calibration_offset = standard_ph - measured_value; // Вычисляем разницу между стандартом и измеренным значением
    this->_calibration += calibration_offset;   // Корректируем калибровку
}
