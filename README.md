# Часы метеостанция
Форк [Ардуино проекта Алекса Гайвера](https://alexgyver.ru/meteoclock/ "Ардуино проекта Алекса Гайвера") под ESP8266
Изначально[ разрабатывался man](https://community.alexgyver.ru/threads/chasy-meteostancija-obsuzhdenie-proekta.1075/post-40150 " разрабатывался man") имел следующие отличия от оригинала:
- Порт на ESP8266 12E
- Синхронизация времени через NTP
- Веб страница часов с данными датчиков и страницей настроек
- Настройки сохраняются в файловую систему LittleFS
- Обновление прошивки "по воздуху" (через вэб страницу часов, но первый раз придется прошить через COM порт)
- Подключение к MQTT брокеру и отправка показаний температуры, давления, влажности, содержания CO2
- После включения подымается WiFi точка 192.168.4.1 (адрес будет на LCD, CLOCK-SSID).
Нужно зайти на страницу и прописать параметри подключения к WiFi, часовой пояс, данные для MQTT брокера
В дальнейшем часы будут получать адрес от вашей сети

В дальнейшем был доработан мной и имеет следующие изменения:
- Добавил отображение графиков и переключение режимов часов в веб интерфейсе из проекта DrDimedrol
- Полностью переписано автоматическое управление яркостью подсветки. Теперь она работает более плавно. Честно говоря, не помню чья была идея, но ему большой респект
- Добавил в веб интерфейс страницу для ручной настройки времени и даты. Может кому пригодится
- Изменил таймаут опроса датчиков до 5 минут. Стандартное значение в 10 секунд у меня на Wemos Mini вызывало жуткие фризы на пару секунд во время каждого опроса. В результате чего время на экране часов довольно сильно начинало отставать от модуле RTC
- По этой же причине уменьшил таймаут синхронизации экранного времени с модулем RTC с 1 раза в час, до 1 раза в минуту
- Изменил алгоритм работы с NTP сервером. Теперь синхронизация проводится каждые 10 минут с помощью библиотеки GyverNTP