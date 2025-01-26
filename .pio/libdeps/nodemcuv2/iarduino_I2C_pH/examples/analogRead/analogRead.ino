// ПРИМЕР ЧТЕНИЯ ДАННЫХ С АНАЛОГОВОГО ВХОДА:      // * Строки со звёздочкой являются необязательными.
                                                  //
// Датчик кислотности жидкости (pH-метр) с щупом: //   https://iarduino.ru/shop/Sensory-Datchiki/datchik-kislotnosti-zhidkosti-ph-metr-flash-i2c.html
// Модуль кислотности жидкости (pH-метр):         //   https://iarduino.ru/shop/Expansion-payments/modul-dlya-datchika-kislotnosti-zhidkosti-ph-metr-flash-i2c.html
// Информация о модуле и описание библиотеки:     //   https://wiki.iarduino.ru/page/line-bumper/
                                                  //
float pHn  = 7.0;                                 //   Нейтральный водородный показатель для датчика. Водородный показатель при котором напряжение с датчика равно 0.
float Vstp = 0.05916;                             //   Шаг изменения напряжения датчика в В, при изменении pH жидкости на 1.0.
float Vn   = 3.3 / 2;                             //   Нейтральное напряжение на выходе модуля. Напряжение на выходе модуля при 7.0 pH.
float Ky   = 3;                                   //   Коэффициент усиления операционного усилителя.
                                                  //
void setup(){                                     //
     delay(500);                                  // * Ждём завершение переходных процессов связанных с подачей питания.
     Serial.begin(9600);                          //   Инициируем работу с шиной UART для передачи данных в монитор последовательного порта на скорости 9600 бит/сек.
}                                                 //
                                                  //
void loop(){                                      //
     float Vout = analogRead(A0)*5.0f/1024.0f;    //   Напряжение на входе A0.
     float pH = (Vn-Vout)/(Vstp*Ky) + pHn;        //
     Serial.print("Кислотность = " );             //
     Serial.print(pH ,1);                         //   Выводим водородный показатель жидкости с 1 знаком после запятой.
     Serial.print(" pH.\r\n"       );             //
     delay(1000);                                 //
}                                                 //
                                                  //
/*   Значения pHn и Vstp определяются в процессе  //
 *   калибровки. Их можно прочитать по шине I2C.  //
 *   float pHn  = sensor.getPHn();                //
 *   float Vstp = sensor.getVstp();               //
 *   float Vn   = sensor.getVn();                 //
 *   float Ky   = sensor.getKy();                 //
 *   float Vout = sensor.getVout();               //
 *   float pH   = sensor.getPH();                 //
 */