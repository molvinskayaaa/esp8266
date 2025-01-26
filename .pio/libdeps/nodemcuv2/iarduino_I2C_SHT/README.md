[![](https://iarduino.ru/img/logo.svg)](https://iarduino.ru)[![](https://wiki.iarduino.ru/img/git-shop.svg?3)](https://iarduino.ru) [![](https://wiki.iarduino.ru/img/git-wiki.svg?2)](https://wiki.iarduino.ru) [![](https://wiki.iarduino.ru/img/git-lesson.svg?2)](https://lesson.iarduino.ru)[![](https://wiki.iarduino.ru/img/git-forum.svg?2)](http://forum.trema.ru)

# iarduino\_I2C\_SHT

**This is a library for Arduino IDE. It allows to work with [Temperture and humidity Sensor](https://iarduino.ru/shop/Sensory-Datchiki/datchik-temperatury-i-vlazhnosti-i2c-trema-modul-v2-0.html) module made by iArduino.ru**

**Данная библиотека для Arduino IDE позволяет работать с модулем [Датчик температуры и влажности](https://iarduino.ru/shop/Sensory-Datchiki/datchik-temperatury-i-vlazhnosti-i2c-trema-modul-v2-0.html) от [iArduino.ru](https://iarduino.ru)**

> Подробнее про установку библиотеки читайте в нашей [инструкции](https://wiki.iarduino.ru/page/Installing_libraries/).

> Подробнее про подключение к [Arduino UNO](https://iarduino.ru/shop/boards/arduino-uno-r3.html)/[Piranha UNO](https://iarduino.ru/shop/boards/piranha-uno-r3.html) читайте на нашей [wiki](https://wiki.iarduino.ru/page/SHT-trema-i2c/#h3_3)


| Модель | Ссылка на магазин |
|---|---|
| <p></p> <img src="https://wiki.iarduino.ru/img/resources/1216/1216.svg" width="100px"></img>| https://iarduino.ru/shop/Sensory-Datchiki/datchik-temperatury-i-vlazhnosti-i2c-trema-modul-v2-0.html |


## Назначение функций:

```C++
#include <iarduino_I2C_SHT> // Подключаем библиотеку iarduino_I2C_SHT для работы с модулем.
```

**Создаём объект** 

```C++
iarduino_I2C_SHT ОБЪЕКТ; // Создаём объект для работы с методами библиотеки без указания адреса модуля на шине I2C.

iarduino_I2C_SHT ОБЪЕКТ( АДРЕС ); // Создаём объект для работы с методами библиотеки указывая адрес модуля на шине I2C.
```

**Инициализация работы** 

```C++
ОБЪЕКТ.begin(); // Инициализация работы с модулем.
```

**Изменение адреса** 

```C++
ОБЪЕКТ.changeAddress( АДРЕС ); // Изменение адреса модуля на шине I2C.
```

**Перезагрузка модуля**

```C++
ОБЪЕКТ.reset(); // Перезагрузка модуля.
```

**Получение текущего адреса** 

```C++
ОБЪЕКТ.getAddress(); // Получение текущего адреса модуля на шине I2C.
```

**Получение версии прошивки** 

```C++
ОБЪЕКТ.getVersion(); // Получение версии прошивки модуля.
```

**Получение температуры в °С** 

```C++
ОБЪЕКТ.getTem(); // Получение температуры окружающей среды в °C.
```

**Получение относительной влажности в %** 

```C++
ОБЪЕКТ.getHum(); // Получение относительной влажности воздуха в %.
```

**Запрос флага изменения температуры** 

```C++
ОБЪЕКТ.getTemChanged(); // Запрос флага указывающего на изменение температуры.
```

**Запрос флага изменения влажности** 

```C++
ОБЪЕКТ.getHumChanged(); // Запрос флага указывающего на изменение влажности.
```

**Установка порога чувствительности изменеия температуры** 

```C++
ОБЪЕКТ.setTemChange( ПОРОГ ); // Порог чувствительности к изменению температуры.
```

**Порог чувствительности изменения влажности** 

```C++
ОБЪЕКТ.setHumChange( ПОРОГ ); // Порог чувствительности к изменению влажности.
```

**Установка периода обновления данных** 

```C++
ОБЪЕКТ.setPeriod( ВРЕМЯ ); // Установка периода обновления данных модулем.
```

