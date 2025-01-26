// ПРИМЕР КАЛИБРОВКИ МОДУЛЯ:                                 // * Строки со звёздочкой являются необязательными.
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
     Serial.print("ОПУСТИТЕ ДАТЧИК В 1 ЖИДКОСТЬ");           //
     Serial.print(" (4.00 pH). \r\n");                       //
     delay(300000);                                          //   Ждём 5 минут.
     sensor.setCalibration(1, 4.00 );                        //   Выполняем 1 стадию калибровки указав известную концентрацию 1 жидкости (в примере 4.0 pH ).
     while(sensor.getCalibration()==1);                      //   Ждём завершения 1 стадии калибровки...
     Serial.print("СПОЛОСНИТЕ ДАТЧИК.\r\n");                 //
     delay( 30000);                                          //   Ждём 30 секунд.
     Serial.print("ОПУСТИТЕ ДАТЧИК В 2 ЖИДКОСТЬ");           //
     Serial.print(" (9.18 pH). \r\n");                       //
     delay(300000);                                          //   Ждём 5 минут.
     sensor.setCalibration(2, 9.18 );                        //   Выполняем 2 стадию калибровки указав известную концентрацию 2 жидкости (в примере 9.18 pH ).
     while(sensor.getCalibration()==2);                      //   Ждём завершения 2 стадии калибровки...
     Serial.print("КАЛИБРОВКА ВЫПОЛНЕНА!\r\n");              //
}                                                            //
                                                             //
void loop(){                                                 //
     Serial.print("Кислотность = " );                        //
     Serial.print(sensor.getPH() ,1);                        //   Выводим водородный показатель жидкости с 1 знаком после запятой.
     Serial.print(" pH.\r\n"       );                        //
     delay(1000);                                            //
}                                                            //