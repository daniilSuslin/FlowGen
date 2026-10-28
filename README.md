# FlowGen

Генератор блок-схем из исходного кода на **C, C++ и Java** для Windows.

## Использование

1. Вставьте код в левое поле или откройте файл (`Файл → Открыть исходный код`, `Ctrl+O`). Кодировки UTF-8 и Windows-1251 определяются автоматически.
2. Выберите функцию в выпадающем списке и нажмите **Построить схему** (`F5`).
3. При необходимости отредактируйте схему (см. ниже).
4. `Файл → Экспорт в PNG` (`Ctrl+E`).

## Что разбирается

`if / else`, `for`, `while`, `do-while`, `switch`, `break`, `continue`, `return`, `try / catch / finally`, цикл по коллекции (`for (x : coll)`).

Операции ввода-вывода (`printf`, `scanf`, `cin`, `cout`, `Scanner`, `System.out` и подобные) выводятся блоками ввода-вывода, например «Ввод: n». Объявления без начального значения (`int a, b;`) по умолчанию скрываются; это отключается в меню `Вид`.

### Ограничения

- блоки `catch` в схему не попадают: строится основная ветка и `finally`;
- лямбды, макросы и `goto` отображаются как обычные блоки-действия;
- в глубоких цепочках `else if` с `break` или `return` стрелка иногда проходит поверх соседнего блока; это исправляется вручную (перенос блока или смена стороны выхода стрелки);

## Сборка из исходников

Нужен компилятор C++11 или новее.

**MinGW-w64:**

```bat
build_mingw.bat
```

## Формат проекта `.flow`

Текстовый файл в UTF-8: заголовок `FLOWGEN`, строки `N` (блоки), `E` (стрелки), `F` (имя функции), `C` (исходный код). Поля разделены табуляцией, переводы строк внутри текста экранированы.

## Лицензия

[MIT](LICENSE)

---

## English summary

FlowGen is a single-file, dependency-free Win32 application that generates editable flowcharts from C, C++ and Java source code. Pick a function, press F5, tweak the diagram by hand (drag blocks, edit text, change block types, add or remove arrows) and export it to PNG. Projects can be saved as `.flow` files. Build with `build_mingw.bat`; prebuilt binaries are attached to the releases.
