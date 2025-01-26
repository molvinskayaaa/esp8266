#ifndef PH4502C_SENSOR_H
#define PH4502C_SENSOR_H

#include <Arduino.h>

class PH4502C_Sensor {
public:
    // Конструктор класса
    PH4502C_Sensor(uint8_t ph_level_pin, uint8_t temp_pin, uint16_t reading_count = 10, uint16_t reading_interval = 100);

    // Метод инициализации
    void init();

    // Метод для чтения уровня pH
    float read_ph_level();

    // Метод для чтения уровня pH (одиночное значение)
    float read_ph_level_single();

    // Метод для чтения температуры
    float read_temp();

    // Метод для калибровки
    void recalibrate(float calibration);

    // Метод для калибровки на основе стандартного раствора
    void calibrate_with_standard(float standard_ph, float measured_value);

private:
    uint8_t _ph_level_pin;       // Пин для pH
    uint8_t _temp_pin;           // Пин для температуры
    uint16_t _reading_count;     // Количество чтений
    uint16_t _reading_interval;   // Интервал между чтениями
    float _calibration;          // Калибровочное значение

    // Константы для работы с pH датчиком
    static const float PH4502C_VOLTAGE;              // Напряжение системы
    static const float PH4502C_MID_VOLTAGE;           // Среднее напряжение для pH 7.00
    static const float PH4502C_PH_VOLTAGE_PER_PH;     // Напряжение на 1 единицу pH
    static const uint16_t _adc_resolution;            // Разрешение АЦП (например, 1023 для 10-битного)
};

#endif // PH4502C_SENSOR_H
