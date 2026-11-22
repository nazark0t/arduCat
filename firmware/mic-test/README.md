# Аудиостенд ESP32-S3

Отдельная голосовая плата N16R8; питание только USB. Моторы отключены.

| Модуль | Подключение |
|---|---|
| I2S-микрофон | VDD=3V3, GND/LR=GND, SCK=GPIO4, WS=GPIO5, SD=GPIO6 |
| MAX98357A | VIN=5V, GND=GND, BCLK=GPIO15, LRC=GPIO16, DIN=GPIO17, SD=GPIO18; GAIN свободен |
| Динамик | 4 Ом 3 Вт между выходами усилителя; ни один выход не соединять с GND |

## Подготовка Windows

Из корня репозитория:

```powershell
python -m venv .tools/esp-venv
.tools/esp-venv/Scripts/python.exe -m pip install platformio==6.2.0 esptool==5.6.0 pyserial==3.5
./firmware/mic-test/build.ps1
```

Сборка использует каталог профиля `.cache/ardubot-pio`, исходники копируются в `.cache/ardubot-mic-test`; артефакты — `.tools/mic-build`. Платформа закреплена в platformio.ini. Скрипт только собирает, не прошивает. Перед первой записью определить именно нужную плату, сохранить полную flash 16 MB, проверить резервную копию. Адреса загрузки и параметры сверить с выводом сборки; не прошивать другую плату по старому COM-номеру.

После отдельной прошивки указать актуальный порт вместо COM_PORT:

```powershell
.tools/esp-venv/Scripts/python.exe firmware/mic-test/capture.py --port COM_PORT --output .tools/recordings/test.wav
.tools/esp-venv/Scripts/python.exe firmware/mic-test/play.py --port COM_PORT --wav .tools/recordings/test.wav --peak 6000
```

Запись запускается явно, 3 секунды, mono PCM16 16 kHz. UART 921600. Воспроизведение начинается после передачи и сверки контрольной суммы (это проверка передачи, не криптографическая аутентификация). RX/TX делят I2S0 по очереди; TX использует два 32-битных слота. Сеть не запускается. Любой с доступом к UART может отправлять команды — это локальный диагностический интерфейс.

Записи содержат голос и остаются локально. Не публикуйте их без отдельной проверки. AUDIO-001: остаётся дребезг динамика; источник не установлен. Известные ограничения SDK описаны в [SECURITY.md](../../SECURITY.md).
