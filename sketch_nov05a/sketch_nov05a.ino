// Раскомментируйте для программной реализации шины I2C: // 
// #define pin_SW_SDA 3 // Назначение любого вывода Arduino для работы в качестве линии SDA программной шины I2C. 
// #define pin_SW_SCL 9 // Назначение любого вывода Arduino для работы в качестве линии SCL программной шины I2C. 
// Раскомментируйте для совместимости с большинством плат: // 
// #include <Wire.h> // Библиотека iarduino_OLED_txt будет использовать методы и функции библиотеки Wire. 
// Ссылки для ознакомления: // 
// Подробная информация о подключении модуля к шине I2C: // http://wiki.iarduino.ru/page/i2c_connection/ 
// Подробная информация о функциях и методах библиотеки: // http://wiki.iarduino.ru/page/OLED_trema/ 
// Библиотека iarduino_OLED_txt (текстовая) экономит ОЗУ: // http://iarduino.ru/file/341.html 
// Бибилиотека iarduino_OLED (графическая): // http://iarduino.ru/file/340.html 
// 
#include <iarduino_OLED_txt.h>          // Подключаем библиотеку iarduino_OLED_txt. 
#include <avr/sleep.h>                  // Подключаем библиотеку сна
iarduino_OLED_txt myOLED(0x3C);         // Объявляем объект myOLED, указывая адрес дисплея на шине I2C: 0x3C или 0x3D. 

extern uint8_t SmallFontRus[];          // Подключаем шрифт SmallFontRus. 
                                        // Определение переменных
volatile unsigned long rounds = 0;      // Объявляем переменную rounds, хранящую количество полных оборотов колеса
volatile unsigned long prev_millis = 0; // Объявляем переменную prev_millis, которая хранит время предыдущего срабатывания датчика оборотов
volatile unsigned long curr_millis = 1; // Объявляем переменную curr_millis, которая хранит время текущего срабатывания датчика оборотов
float S = 100;                          // Пройденное расстояние за 1 оборот в сантиметрах, равняется периметру колеса
float t = 0;                            // Время оборота в миллисекундах
int V = 0;                              // Скорочть в КМ/Ч
volatile unsigned long dist = 0;        // Всего пройденное расстояние

//=============================================== Функция настройки
void setup(){                           // Первичная настройка параметров
  pinMode(2,INPUT_PULLUP);              // Вывод D2 ардуино определяем как вход, с внутренним подтягивающим резистором к высокому уровню
  attachInterrupt(0, inc, FALLING);     // Настраиваем прерывание на выводе D2, вызывающее фцнкцию inc, при падении уровня с HIGH до LOW
  myOLED.begin();                       // Инициируем работу с дисплеем. 
  myOLED.setFont(SmallFontRus);         // Указываем шрифт который требуется использовать для вывода цифр и текста.  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // Настраиваем режим энергосбережения
} 

//=============================================== Функция главная
void loop(){                                    // Основной метод программы
  if (millis()%1000 == 0) {                     // Обновление дисплея будет происходить каждую секунду
    if (millis()-prev_millis < 3000) {          // Если получен сигнал не более 3 секунд назад, вычисляем скорость, иначе скорость 0 
      t = (curr_millis - prev_millis)/36;       // После упрощения формулы вычисляем t для функции V=S/t
      V = round(S/t);                           //Вычисляем скорость в КМ/Ч
    }
    else                                        // Если не получен сигнал больше 3 секунд, то
    {
      V = 0;                                    // Скорость 0 КМ/Ч
    }
    dist = round(S*rounds/100);                 // Вычисляем пройденное расстояние
    writeDisplay();                             // Выводим на дисплей информацию
    if (millis()-curr_millis > 30000) {         // Если мы стоим больше 30 секунд, то
      myOLED.clrScr();                          // Очищаем дисплей
      Sleep_on();                               // Переходим в режим сна
    }
  } 
} 

//=============================================== Функция при срабатывании датчика оборотов
void inc() {                  // При получении сигнала с датчика:
  rounds++;                   // Увеличиваем количество оборотв на 1
  prev_millis = curr_millis;  // Определяем предыдущее время срабатывание датчика
  curr_millis = millis();     // Определяем текущее время срабатывания датчика
}

//=============================================== Функция вывода информации о поездке
void writeDisplay () {
  myOLED.print("ROUNDS = ", OLED_C, 1);       // Выводим текст по центру 1 строки. Текст будет написан белыми буквами на чёрном фоне. 
  myOLED.print(rounds, OLED_C, 2);            // Выводим количество оборотов по центру 2 строки. Текст будет написан белыми буквами на чёрном фоне.
  myOLED.print("SPEED (KM/H) = ", OLED_C, 3); // Выводим текст по центру 3 строки. Текст будет написан белыми буквами на чёрном фоне.
  myOLED.print("   ", OLED_C, 4);             // Выводим пустой текст по центру 4 строки (для удаления артефактов предыдущего вывода). Текст будет написан белыми буквами на чёрном фоне.
  myOLED.print(V, OLED_C, 4);                 // Выводим скорость по центру 4 строки. Текст будет написан белыми буквами на чёрном фоне.
  myOLED.print("DISTANCE (M)= ", OLED_C, 5);  // Выводим текст по центру 5 строки. Текст будет написан белыми буквами на чёрном фоне.
  myOLED.print(dist, OLED_C, 6);              // Выводим пройденное расстояние по центру 6 строки. Текст будет написан белыми буквами на чёрном фоне.
}

//=============================================== Функция засыпания
void Sleep_on(){
  rounds = 0;                                     // Обнуляем переменную rounds, хранящую количество полных оборотов колеса
  dist = 0;                                       // Обнуляем пройденное расстояние
  attachInterrupt(0, wakeon, FALLING);            // Настраиваем прерывание на выводе D2, вызывающее фцнкцию wakeon, при падении уровня с HIGH до LOW 
  sleep_enable();                                 // Разрешаем спящий режим
  sleep_mode();                                   // Засыпаем
}

//=============================================== Функция просыпания
void wakeon(){                                    
  sleep_disable();                                // Запрещаем спящий режим
  detachInterrupt(0);                             // Отключаем прерывания
  curr_millis = millis();                         // Определяем последнее срабатывание датчика оборотов (для спящего режима через 30 сек)
  attachInterrupt(0, inc, FALLING);               // Настраиваем прерывание на выводе D2, вызывающее фцнкцию inc, при падении уровня с HIGH до LOW
} 
