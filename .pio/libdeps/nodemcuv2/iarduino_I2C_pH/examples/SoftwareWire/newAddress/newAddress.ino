// ПРИМЕР СМЕНЫ АДРЕСА МОДУЛЯ:                               // * Строки со звёздочкой являются необязательными.
// Требуется установить библиотеку <iarduino_I2C_Software.h> //   https://iarduino.ru/file/627.html
                                                             //
// Датчик кислотности жидкости (pH-метр) с щупом:            //   https://iarduino.ru/shop/Sensory-Datchiki/datchik-kislotnosti-zhidkosti-ph-metr-flash-i2c.html
// Модуль кислотности жидкости (pH-метр):                    //   https://iarduino.ru/shop/Expansion-payments/modul-dlya-datchika-kislotnosti-zhidkosti-ph-metr-flash-i2c.html
// Информация о модуле и описание библиотеки:                //   https://wiki.iarduino.ru/page/line-bumper/
                                                             //
// Данный скетч демонстрирует не только                      //
// возможность смены адреса на указанный в                   //
// переменной newAddress, но и обнаружение,                  //
// и вывод текущего адреса модуля на шине I2C.               //
                                                             //
// Библиотека <iarduino_I2C_pH.h> предназначена              //
// для работы с pH-метрами I2C-flash                         //
// и меняет только их адреса.                                //
// Но, в разделе Файл/Примеры/.../findSortDevices            //
// находятся примеры смены и сортировки адресов              //
// любых модулей iarduino серии Flash I2C.                   //
                                                             //
uint8_t newAddress = 0x09;                                   //   Назначаемый модулю адрес (0x07 < адрес < 0x7F).
                                                             //
#include <iarduino_I2C_Software.h>                           //   Подключаем библиотеку для работы с программной шиной I2C, до подключения библиотеки iarduino_I2C_pH.
SoftTwoWire sWire(3,4);                                      //   Создаём объект программной шины I2C указав выводы которым будет назначена роль линий: SDA, SCL.
                                                             //
#include <iarduino_I2C_pH.h>                                 //   Подключаем библиотеку   для работы с pH-метром I2C-flash.
iarduino_I2C_pH sensor;                                      //   Объявляем объект sensor для работы с функциями и методами библиотеки iarduino_I2C_pH.
                                                             //   Если при объявлении объекта указать адрес, например, iarduino_I2C_pH sensor(0xBB), то пример будет работать с тем модулем, адрес которого был указан.
void setup(){                                                //
    delay(500);                                              // * Ждём завершение переходных процессов связанных с подачей питания.
    Serial.begin(9600);                                      //
    while(!Serial){;}                                        // * Ждём завершения инициализации шины UART.
    if( sensor.begin(&sWire) ){                              //   Инициируем работу с модулем, указав ссылку на объект для работы с шиной I2C на которой находится модуль (Wire, Wire1, Wire2 ...).
        Serial.print("Найден модуль 0x");                    //
        Serial.println(sensor.getAddress(), HEX);            //   Выводим текущий адрес модуля.
        if( sensor.changeAddress(newAddress) ){              //   Меняем адрес модуля на newAddress.
            Serial.print("Адрес изменён на 0x");             //
            Serial.println(sensor.getAddress(),HEX);         // Выводим текущий адрес модуля.
        }else{                                               //
            Serial.println("Адрес не изменён!");             //
        }                                                    //
    }else{                                                   //
        Serial.println("Модуль не найден!");                 //
    }                                                        //
}                                                            //
                                                             //
void loop(){                                                 //
}                                                            //
