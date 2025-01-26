// ПРИМЕР ПОЛУЧЕНИЯ И ВЫЧИСЛЕНИЯ ДАННЫХ:                     // * Строки со звёздочкой являются необязательными.
// Требуется установить библиотеку <iarduino_I2C_Software.h> //   https://iarduino.ru/file/627.html
                                                             //
// Датчик кислотности жидкости (pH-метр) с щупом:            //   https://iarduino.ru/shop/Sensory-Datchiki/datchik-kislotnosti-zhidkosti-ph-metr-flash-i2c.html
// Модуль кислотности жидкости (pH-метр):                    //   https://iarduino.ru/shop/Expansion-payments/modul-dlya-datchika-kislotnosti-zhidkosti-ph-metr-flash-i2c.html
// Информация о модуле и описание библиотеки:                //   https://wiki.iarduino.ru/page/line-bumper/
                                                             //
#include <iarduino_I2C_Software.h>                           //   Подключаем библиотеку для работы с программной шиной I2C, до подключения библиотеки iarduino_I2C_pH.
SoftTwoWire sWire(3,4);                                      //   Создаём объект программной шины I2C указав выводы которым будет назначена роль линий: SDA, SCL.
                                                             //
#include <iarduino_I2C_pH.h>                                 //   Подключаем библиотеку   для работы с pH-метром I2C-flash.
iarduino_I2C_pH sensor(0x09);                                //   Объявляем объект sensor для работы с функциями и методами библиотеки iarduino_I2C_pH, указывая адрес модуля на шине I2C.
                                                             //   Если объявить объект без указания адреса (iarduino_I2C_pH sensor;), то адрес будет найден автоматически.
void setup(){                                                //
     delay(500);                                             // * Ждём завершение переходных процессов связанных с подачей питания.
     Serial.begin(9600);                                     //   Инициируем работу с шиной UART для передачи данных в монитор последовательного порта на скорости 9600 бит/сек.
     sensor.begin(&sWire);                                   //   Инициируем работу с pH-метром, указав ссылку на объект для работы с шиной I2C на которой находится модуль (по умолчанию &Wire).
}                                                            //
                                                             //
void loop(){                                                 //
     Serial.print("Кислотность = pHn+"      );               //
     Serial.print("( Vn -Vout)/( Vstep *Ky)");               //
     Serial.print("\r\n            = "      );               //
     Serial.print(sensor.getPHn()        , 1);               //
     Serial.print("+("                      );               //
     Serial.print(sensor.getVn()         , 2);               //
     Serial.print("-"                       );               //
     Serial.print(sensor.getVout()       , 2);               //
     Serial.print(")/("                     );               //
     Serial.print(sensor.getVstp()/1000  , 5);               //
     Serial.print("* "                      );               //
     Serial.print(sensor.getKy()         , 0);               //
     Serial.print(") = "                    );               //
     Serial.print(sensor.getPH()         , 1);               //
     Serial.print(" pH.\r\n"                );               //
     delay(1000);                                            //
}                                                            //