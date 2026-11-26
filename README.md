# ChessForge 3D

Нативные офлайн-шахматы для **Anbernic RG35XX Plus** и **MustardOS 2606.0 Andromeda**. Игра написана на C++17 и SDL2: ей не нужен браузер, интернет или телефон.

## Что готово

- Полные шахматные правила: рокировка, en passant, превращение, шах, мат, пат и правило 50 ходов.
- `Stockfish` через UCI из `bin/stockfish`. Если Stockfish отсутствует, включается встроенный minimax-бот.
- Уровень противника от 400 до 3200. Значения 1320–3190 передаются Stockfish через `UCI_LimitStrength` и `UCI_Elo`.
- Настоящая перспективная 3D-сцена, объёмные фигуры, камера с поворотом и наклоном. Рендер программный, поэтому не зависит от OpenGL-драйвера прошивки.
- 2D-режим, сохранение FEN, автосохранение и отмена хода.
- Четыре готовые палитры: Oak, Violet, Forest, Mono.
- Полностью настраиваемая палитра: свой цвет светлых клеток, тёмных клеток, акцента и фона — RGB от 0 до 255.
- Пять процедурных фактур, доступных в 2D и 3D: `Flat`, `Wood`, `Marble`, `Carbon`, `Felt`.

## Настройка цветов и фактур на консоли

1. Запусти игру и нажми **Y**.
2. В `THEME` поставь `CUSTOM`, либо открой любой RGB-пункт — игра сама переключится на `CUSTOM`.
3. В `TEXTURE` выбери фактуру доски.
4. Выбери `LIGHT RGB`, `DARK RGB`, `ACCENT RGB` или `BACK RGB`.
5. Нажми **A**: `Up/Down` выбирают канал R/G/B, `Left/Right` меняют значение на 5.
6. Нажми **A**, чтобы сохранить цвет. Настройки сохраняются автоматически.

Можно также вручную задать палитру в `ports/chessforge3d/saves/settings.cfg`. Готовый пример неоновой карбоновой темы — [`themes/neon-carbon.cfg`](themes/neon-carbon.cfg).

## Управление

| RG35XX Plus | Действие |
|---|---|
| D-pad | Курсор на доске / выбор настройки |
| A | Выбрать фигуру, сделать ход, редактировать RGB |
| B | Отменить выбор или последний ход; закрыть настройки |
| X | Переключить 2D / 3D |
| Y | Открыть/закрыть настройки |
| Start | Новая партия |
| Select / H | Экран помощи |
| Menu | Выход |
| L1 / R1 | Поворот камеры |
| Q / E | Поворот камеры при тесте с клавиатурой |

## Установка на muOS (MustardOS)

Игра ставится как **приложение** (`.muxapp`), а не как port:

1. Собери пакет: `scripts/package.sh` (или скачай артефакт `ChessForge3D.muxapp` из GitHub Actions).
2. Скопируй `ChessForge3D.muxapp` в `<SD>/MUOS/ARCHIVE/`.
3. На консоли: **Applications → Archive Manager** → выбери файл → **A**.
4. Запусти **Applications → ChessForge 3D**.

Вручную: распакуй архив так, чтобы получилось `<SD>/MUOS/application/ChessForge3D/mux_launch.sh`.
Stockfish (ARM64) кладётся в `bin/stockfish`; CI собирает его сам.

## Если кнопки не работают

Игра понимает геймпад (SDL GameController, запасной вариант — сырой joystick). **Menu/Guide** — выход.
Если A и B перепутаны — поставь `nintendo_layout=0` в `saves/input.cfg` (пример: `input.cfg.example`).
Для диагностики создай пустой файл `saves/input_debug`, понажимай кнопки и посмотри `saves/input.log`.

## Сборка ARM64

Проще всего через GitHub Actions (`.github/workflows/build.yml`, нативный ARM64-раннер).
Локально на ARM64 Linux с libsdl2-dev:

```sh
cmake -S . -B build-rg35xx -DCMAKE_BUILD_TYPE=Release
cmake --build build-rg35xx -j4
./scripts/package.sh build-rg35xx
```

## Тесты правил

```sh
cmake -S . -B build-tests -DCHESSFORGE_BUILD_TESTS=ON
cmake --build build-tests
./build-tests/chess_rules_tests
```

## Совместимость

Проверялось на Anbernic RG35XX Plus с MustardOS 2606.0 Andromeda. На других устройствах и прошивках не тестировалось.

## Credits

- Шахматный движок: [Stockfish](https://github.com/official-stockfish/Stockfish) (GPLv3). Если в пакет попал `bin/stockfish`, он запускается как отдельная программа; его исходники и лицензия по ссылке.
- Графика и ввод: [SDL2](https://www.libsdl.org/) (zlib license).
- Код ChessForge 3D распространяется по лицензии MIT, см. `LICENSE`.

## Наборы фигур

В игре: **Y → PIECES** (влево/вправо) переключает набор. «BUILT-IN» это встроенные векторные фигуры.
Свои наборы кладутся в `pieces/<имя>/` (файлы `wP.bmp … bK.bmp`, подробности в `pieces/README.md`).
Релизная сборка включает набор `cburnett` с lichess (автор Colin M.L. Burnett; лицензию проверяйте в репозитории
[lila](https://github.com/lichess-org/lila/tree/master/public/piece)).
