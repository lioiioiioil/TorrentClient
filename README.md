# TorrentClient

Учебный консольный BitTorrent-клиент на C++20 для Linux. Разбирает метаданные
`.torrent`, получает пиров через HTTP-трекер и загружает части файла в отдельных
потоках. Для обмена с пирами используются TCP-сокеты и `poll`, для проверки
целостности частей — SHA-1 из OpenSSL.

## Сборка

Нужны компилятор с C++20, CMake 3.20+, Git, OpenSSL и libcurl. Для Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git libssl-dev libcurl4-openssl-dev
git clone https://github.com/lioiioiioil/TorrentClient.git
cd TorrentClient
cmake -S torrent-client-prototype -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

При первой конфигурации CMake скачивает закреплённую версию CPR. Обычный `make`
в корне только собирает проект и не начинает загрузку торрента.

## Запуск

Запускайте клиент из корня репозитория, соблюдая порядок аргументов:

```bash
./build/torrent-client-prototype -d ./downloads -p 100 /path/to/file.torrent
```

- `-d` — каталог для загружаемого файла, создаётся при необходимости;
- `-p` — доля частей для загрузки в процентах; для целого файла используйте `100`;
- последний аргумент — путь к однофайловому `.torrent`.

Команда обращается к трекеру и пирам; доступность загрузки зависит от них.
Выходной файл создаётся или перезаписывается в указанном каталоге.

## Проверки без сети

Для тестов не требуются CPR, трекер или запущенные пиры:

```bash
cmake -S torrent-client-prototype -B build-tests -DBUILD_CLIENT=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

Проверяются вложенные bencode-словари и бинарные строки, формат сообщений,
известные SHA-1 векторы, запись частей по смещениям и повторная постановка
повреждённой части в очередь. Файлы создаются только во временном каталоге теста.
Для GCC/Clang можно добавить `-DENABLE_SANITIZERS=ON`, чтобы включить ASan/UBSan.
GitHub Actions собирает клиент и запускает эти проверки на Ubuntu.

## Структура

- `torrent-client-prototype/` — исходники клиента и CMake;
- `torrent-client-prototype/tests/` — локальные проверки без сетевого обмена;
- `resources/` — пример `.torrent` из исходного учебного проекта;
- `dir/` — исходный каталог для результатов; в примере запуска используется `downloads/`.

## Ограничения

Это учебная реализация, а не полноценный torrent-клиент. Нет magnet-ссылок,
DHT, UDP-трекеров, многофайловых торрентов и возобновления после перезапуска.
Встроенная итоговая проверка в `main.cpp` пока ожидает торрент не менее чем
из 200 частей. Парсер ориентирован на корректные учебные `.torrent`;
обработка произвольных повреждённых метаданных не покрыта тестами.
