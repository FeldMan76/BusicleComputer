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
#include <EEPROM.h>                     // Подключаем библиотеку EEPROM
iarduino_OLED_txt myOLED(0x3C);         // Объявляем объект myOLED, указывая адрес дисплея на шине I2C: 0x3C или 0x3D.

#define btnMode 3                       // Кнопка смена режимов на порту D3 
#define btnEnter 4                      // Кнопка подтверждения действия на порту D4 

extern uint8_t SmallFontRus[];          // Подключаем шрифт SmallFontRus.

volatile unsigned long rounds = 0;      // Хранит количество полных оборотов колеса за поездку
volatile unsigned long prev_millis = 0; // Хранит время предыдущего срабатывания датчика оборотов
volatile unsigned long curr_millis = 1; // Хранит время текущего срабатывания датчика оборотов
int S = 0;                              // Пройденное расстояние за 1 оборот в сантиметрах, равняется периметру колеса
unsigned long t = 0;                    // Время оборота в миллисекундах
int Vms = 0;                            // Скорость в м/с
int V = 0;                              // Скорочть в КМ/ч
volatile unsigned long currDist = 0;    // Пройденное расстояние за поездку
unsigned long getDist = 0;              // Пройденное расстояние из EEPROM
unsigned long allDist = 0;              // Всего пройденное расстояние
int eeAddressDist = 0;                  // Начальный адрес EEPROM, хранящий расстояние всего (0-3 : 4 байта для unsigned long)
int eeAddressWihle = 5;                 // Начальный адрес EEPROM, хранящий радиус колеса    (5-8 : 4 байта для float)
float radius = 0;                       // Радиус колеса в дюймах
boolean setMode = false;                // Признак входа в режим установки
volatile unsigned long Set_millis = 0;  // Хранит время последнего нажатия на клавишу
byte curMode = 1;                       // Текущая позиция курсора

//=============================================== Функция настройки
void setup() {                                  // Первичная настройка параметров
  pinMode(2, INPUT_PULLUP);                     // Вывод D2 ардуино определяем как вход, с внутренним подтягивающим резистором к высокому уровню
  pinMode(3, INPUT_PULLUP);                     // Вывод D2 ардуино определяем как вход, с внутренним подтягивающим резистором к высокому уровню
  pinMode(4, INPUT_PULLUP);                     // Вывод D2 ардуино определяем как вход, с внутренним подтягивающим резистором к высокому уровню
  attachInterrupt(0, inc, FALLING);             // Настраиваем прерывание на выводе D2, вызывающее фцнкцию inc, при падении уровня с HIGH до LOW
  myOLED.begin();                               // Инициируем работу с дисплеем.
  myOLED.setFont(SmallFontRus);                 // Указываем шрифт который требуется использовать для вывода цифр и текста.
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);          // Настраиваем режим энергосбережения
  EEPROM.get( eeAddressWihle, radius);          // Получаем радиус колеса, сохраненный в памяти
  readDist();                                   // Выводим на экран расстояние всего
  S = round(2 * 3.415926535 * radius * 0.025);  // Вычисляем периметр колеса в метрах (пройдено за 1 оборот)
}

//=============================================== Функция главная
void loop() {                                         // Основной метод программы
  if (setMode == false) {                             // Если не запущен режим изменения радиуса колеса, то
    if (digitalRead(btnMode) == LOW) {                // Проверяем, нажата ли кнопка смены режима, если нажата, то
      myOLED.clrScr();                                // Очищаем дисплей
      toSet();                                        // Переходим в режим редактирования радиуса
    }
    if (millis() % 1000 == 0) {                       // Обновление дисплея будет происходить каждую секунду
      if (millis() - prev_millis < 3000) {            // Если получен сигнал не более 3 секунд назад, вычисляем скорость, иначе скорость 0
        t = (curr_millis - prev_millis);              // Вычисляем время оборота в миллисекундах
        Vms = round(S * 1000 / t);                    // Вычисляем скорость в м/с
        V = round(Vms * 3.6);                         // Вычисляем скорость в КМ/Ч
      }
      else                                            // Если не получен сигнал больше 3 секунд, то
      {
        V = 0;                                        // Скорость 0 КМ/Ч
      }
      currDist = round(S * rounds);                   // Вычисляем пройденное расстояние в метрах
      writeDisplay();                                 // Выводим на дисплей информацию
      if (millis() - curr_millis > 30000 and millis() - Set_millis > 10000) { // Если мы стоим больше 30 секунд, и не нажимали кнопки более 10 сек
        myOLED.clrScr();                              // Очищаем дисплей
        saveDist();                                   // Обновляем дистанцию всего
        Sleep_on();                                   // Переходим в режим сна
      }
    }
  }
  else                                                // Если режим редактирования запущен
  {
    if (digitalRead(btnMode) == LOW) {                // Проверяем, нажата ли кнопка смены режима, если нажата, то
      Set_millis = millis();                          // Обновляем время последнего нажатия кнопки
      myOLED.clrScr();                                // Очищаем дисплей
      if (curMode < 3) { curMode++; }                 // Если выбран 3 пункт меню, то
      else { curMode = 1; }                           // Переходим к первому пункту
    }
    writeDispSetMode(curMode);

    if (digitalRead(btnEnter) == LOW) {               // Если нажата клавиша подтверждения, то 
      Set_millis = millis();                          // Обновляем время последнего нажатия кнопки
      if (curMode == 1) {                             // Если стоим на 1 пункте меню, то
        myOLED.print("           ", OLED_C, 2);       // Очищаем предыдущие значения радиуса во 2 строке
        if (radius < 30) {                            // Если радиус менее 30, то
          radius = radius + 0.5;                      // Прибавляем 0,5 к радиусу
        }
        else                                          // Если радиус более или равен 30
        {
          radius = 1;                                 // Устанавливаем радиус в 1
        }
        delay(200);                                   // Ждем 0,2 секунды, чтобы слишком много раз условие не сработало при нажатии кнопки
      }
      if (curMode == 2) {                             // Если выбран пункт отмены, то
        EEPROM.get( eeAddressWihle, radius);          // Получаем старое значение радиуса из EEPROM
        exitSet();                                    // Выходим из меню настройки
      }
      if (curMode == 3) {                             // Если выбран пункт меню подтверждения, то
        EEPROM.put( eeAddressWihle, radius);          // Сохраняем установленное значение в EEPROM
        exitSet();                                    // Выходим из меню настройки    
      }
    }

    if (millis() - Set_millis > 10000) {              // В меню настройки не было действий в течении 10 секунд, то
      EEPROM.get( eeAddressWihle, radius);            // Получаем старое значение радиуса из EEPROM
      exitSet();                                      // Выходим из меню настройки
    }
  }
}

//=============================================== Функция при срабатывании датчика оборотов
void inc() {
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
  myOLED.print(currDist, OLED_C, 6);          // Выводим пройденное расстояние по центру 6 строки. Текст будет написан белыми буквами на чёрном фоне.
}

//=============================================== Функция сохранения в EEPROM
void saveDist() {
  EEPROM.get( eeAddressDist, getDist);    // Получение сохраненной в памяти дистанции всего
  allDist = getDist + currDist;           // Прибавление к полученному результату дистанции за поездку
  EEPROM.put(eeAddressDist, allDist);     // Обновление Содержащегося в памяти значения дистанции всего
}

//=============================================== Функция чтения из EEPROM
void readDist() {
  myOLED.print("ALL DIST", OLED_C, 1);  // Выводим текст по центру 1 строки. Текст будет написан белыми буквами на чёрном фоне.
  EEPROM.get( eeAddressDist, getDist);  // Получаем пройденное расстояние всего из памяти
  myOLED.print(getDist, OLED_C, 2);     // Выводим полученное расстояние по центру 2 строки. Текст будет написан белыми буквами на чёрном фоне.
  myOLED.print("RADIUS", OLED_C, 3);    // Выводим текст по центру 3 строки. Текст будет написан белыми буквами на чёрном фоне.
  myOLED.print(radius, OLED_C, 4);      // Выводим радиус колеса полученный из EEPROM
  myOLED.print("WELCOME", OLED_C, 6);   // Выводим радиус колеса полученный из EEPROM
  delay(2000);                          // Показываем 2 секунды 
  myOLED.clrScr();                      // Очищаем дисплей
}

//=============================================== Функция засыпания
void Sleep_on() {
  rounds = 0;                           // Обнуляем переменную rounds, хранящую количество полных оборотов колеса
  currDist = 0;                         // Обнуляем пройденное расстояние
  attachInterrupt(0, wakeon, FALLING);  // Настраиваем прерывание на выводе D2, вызывающее фцнкцию wakeon, при падении уровня с HIGH до LOW
  sleep_enable();                       // Разрешаем спящий режим
  sleep_mode();                         // Засыпаем
}

//=============================================== Функция просыпания
void wakeon() {
  sleep_disable();                  // Запрещаем спящий режим
  detachInterrupt(0);               // Отключаем прерывания
  curr_millis = millis();           // Определяем последнее срабатывание датчика оборотов (для спящего режима через 30 сек)
  attachInterrupt(0, inc, FALLING); // Настраиваем прерывание на выводе D2, вызывающее фцнкцию inc, при падении уровня с HIGH до LOW
}

//=============================================== Функция входа в режим настройки
void toSet() {
  setMode = true;         // Перейти в режим изменения радиуса
  Set_millis = millis();  // Обновляем время последнего нажатия кнопки
  setMode = 1;            // Установка первого выбранного пункта меню 
}

//=============================================== Функция выхода из режима настройки
void exitSet() {
  setMode = false;  // Выходим из режима настойки
  myOLED.clrScr();  // Очищаем дисплей
}

//=============================================== Функция вывода меню настройки в зависимости от выбранного пункта
void writeDispSetMode(byte curMode) {
  if (curMode == 1) {
    myOLED.print(">> SET RADIUS +0.5   ", OLED_C, 1); // Выводим текст по центру 1 строки. Текст будет написан белыми буквами на чёрном фоне.
    myOLED.print(radius, OLED_C, 2);                  // Выводим радиус колеса во 2 строку
    myOLED.print("CANCEL", OLED_C, 3);                // Выводим текст по центру 3 строки. Текст будет написан белыми буквами на чёрном фоне. 
    myOLED.print("SAVE", OLED_C, 4);                  // Выводим текст по центру 4 строки. Текст будет написан белыми буквами на чёрном фоне.
  }
  if (curMode == 2) {
    myOLED.print("SET RADIUS +0.5", OLED_C, 1);       // Выводим текст по центру 1 строки. Текст будет написан белыми буквами на чёрном фоне.
    myOLED.print(radius, OLED_C, 2);                  // Выводим радиус колеса во 2 строку
    myOLED.print(">> CANCEL   ", OLED_C, 3);          // Выводим текст по центру 3 строки. Текст будет написан белыми буквами на чёрном фоне.
    myOLED.print("SAVE", OLED_C, 4);                  // Выводим текст по центру 4 строки. Текст будет написан белыми буквами на чёрном фоне.
  }
  if (curMode == 3) {
    myOLED.print("SET RADIUS +0.5", OLED_C, 1);       // Выводим текст по центру 1 строки. Текст будет написан белыми буквами на чёрном фоне.
    myOLED.print(radius, OLED_C, 2);                  // Выводим радиус колеса во 2 строку
    myOLED.print("CANCEL", OLED_C, 3);                // Выводим текст по центру 3 строки. Текст будет написан белыми буквами на чёрном фоне.
    myOLED.print(">> SAVE   ", OLED_C, 4);            // Выводим текст по центру 4 строки. Текст будет написан белыми буквами на чёрном фоне.
  }
}
