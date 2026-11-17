# lvml

Разметка (`.lvml`) и таблицы стилей (`.lvss`) для [LVGL](https://lvgl.io) 9.5 на C++17.
HTML/CSS-подобный синтаксис, но свойства и виджеты — LVGL: сцена строится в обычное дерево
`lv_obj_t`, стили компилируются в `lv_style_t` (по одному на правило), состояния (`:hover`,
`:pressed`, `:checked`…) и части (`::scrollbar`, `::indicator`…) переключает сам LVGL.

```
source/             библиотека (по классу на файл: Foo.cpp → CFoo)
  ядро, без LVGL:   CLvmlParser CLvmlDocument CLvmlNode CLvssParser CLvssStyleSheet
                    CLvssSelector CLvssRule CLvssDeclaration CLvssValue CTextUtils
  рантайм (LVGL):   CLvmlContext CLvmlScene CLvmlBuilder CLvmlStyleSet CLvssProperty
                    CLvmlWidgetFactory CLvmlFontManager CLvmlFileSystem CLvmlStorage CLvmlEvent
                    CLvmlResources / CLvmlDirectoryResources / CLvmlMemoryResources
platform/win32/     CLvmlWin32Window — окно без рамки для драйвера LV_USE_WINDOWS
tools/html2lvml/    конвертер HTML + CSS → .lvml + .lvss
tools/lvml-render/  headless-рендер сцены в BMP (превью, проверка вёрстки, дамп дерева)
tests/              тесты парсеров
examples/lv_conf.h  конфиг LVGL, на котором всё проверено
vendor/lvgl         LVGL v9.5.0 (submodule)
```

## Пример

```xml
<lvml>
<head>
    <link rel="stylesheet" href="../common.lvss"/>
    <style>
        #ok:hover { image-recolor: #ccffcc; image-recolor-opa: 25%; }
    </style>
</head>
<screen>
    <image id="background" src="window.png"/>
    <obj id="row" checkable checked>
        <image id="box-on" src="on.png"/>
        <label>Создать ярлык</label>
    </obj>
    <image id="ok" src="ok.png" onclick="submit" next="02/scene.lvml"/>
</screen>
</lvml>
```

```cpp
lv_init();
CLvmlWin32Window window;                       // или любой свой lv_display_t
window.Create(L"App", 1000, 600);

CLvmlContext ui(std::make_shared<CLvmlDirectoryResources>("ui"));   // или CLvmlMemoryResources
ui.SetScale(window.GetScale());                // px в .lvss — это px при 96 DPI
auto scene = ui.LoadScene("01/scene.lvml");
scene->On("submit", [&](CLvmlEvent& e) { next = e.GetAttribute("next"); });
scene->SetText("status", "Готово");
scene->SetVisible("modal", true);
scene->AddClass("row", "active");              // перестилизация поддерева
```

Сцену нельзя уничтожать из её же обработчика — запомните действие и загрузите следующую сцену на
следующей итерации цикла (так сделан installer).

## .lvml

* Корень `<lvml>`, в `<head>`: `<title>`, `<link rel="stylesheet" href>` (или `<style src>`),
  `<style>…</style>`, `<font family="inter" src="Inter.ttf"/>`. Тело — `<screen>` (или `<body>`),
  он растягивается на 100 % родителя.
* Теги → виджеты: `obj`/`div`/`view`/`panel`, `label`/`text`, `button`, `image`/`img` (может иметь детей),
  `checkbox`, `bar`/`progress`, `slider`, `switch`, `textarea`/`input`, `dropdown`/`select` с `<option>`,
  `richtext` (спаны: `<span>`, `<strong>`, `<a href>`, `<br/>`). HTML-теги (`p`, `h1`…`h6`, `span`, `a`,
  `ul`, `li`, `header`…) — контейнеры, текст внутри становится меткой. Свои теги:
  `CLvmlWidgetFactory::Register()`.
* Атрибуты: `id`, `class`, `style`, `hidden`, `checked`, `checkable`, `disabled`, `clickable`,
  `scrollable`, `theme="keep"` (оставить тему LVGL; по умолчанию все стили темы снимаются),
  `src`, `text`, `min`/`max`/`value`, `placeholder`, `one-line`, `password`, `href`.
* События: `onclick`, `onchange`, `onpress`, `onrelease`, `onlongpress`, `onfocus`, `onblur`,
  `onready`, `onhover`, `onleave`, `onscroll` — значение атрибута = имя действия в `CLvmlScene::On()`.
  `href` без обработчика даёт действие `link`.
* По умолчанию раскладка абсолютная, как в LVGL (`x`/`y`/`align`); поток — через `layout: flex`.
  Контейнеры имеют размер по содержимому и не прокручиваются, пока не задан `overflow`.

## .lvss

Синтаксис CSS: комментарии `/* */`, списки селекторов, `@import "x.lvss";`,
`@font-face { font-family; src: url(x.ttf) }`, переменные `:root { --a: #fff }` + `var(--a, fallback)`.

**Селекторы:** `tag`, `#id`, `.class`, `*`, потомок ` `, ребёнок `>`, `:first-child`, `:last-child`,
`:only-child`; состояния `:hover`/`:hovered`, `:active`/`:pressed`, `:checked`, `:focus`/`:focused`,
`:focus-visible`, `:disabled`, `:edited`, `:scrolled`, `:user1..4`; части `::scrollbar`,
`::indicator`, `::knob`, `::selected`, `::items`, `::cursor`. Состояние на предке
(`#row:checked #box { display: block }`) тоже работает: сцена перестилизует поддерево при смене
состояния. Каскад: специфичность, затем порядок; `style=""` важнее всего. `!important` не учитывается.

**Свойства** — имена стилей LVGL через дефис плюс привычные CSS-имена:

| группа | свойства |
|---|---|
| размер, позиция | `width` `height` `min-*` `max-*` (`px`, `%`, `content`/`auto`, `rem`, `em`, `vw`, `vh`), `x`/`left`, `y`/`top`, `align` (`top-left`…`center`, `out-*`), `translate-x/y`, `transform` (`translate/scale/rotate`), `transform-scale-x/y`, `transform-rotation`, `transform-pivot-x/y`, `size` |
| отступы | `padding*`/`pad-*`, `margin*`, `gap`, `row-gap`/`pad-row`, `column-gap`/`pad-column` |
| фон | `background-color`/`bg-color` (альфа в `#rrggbbaa` → `bg-opa`), `bg-opa`, `background` (цвет / `url()` / `linear-gradient()`), `bg-image`, `bg-image-recolor(-opa)`, `bg-image-tiled`, `bg-grad-*` |
| рамка, тень | `border`, `border-width/color/opa/side`, `border-top/bottom/left/right`, `border-radius`/`radius` (`50%` → круг), `outline*`, `box-shadow`, `shadow-*`, `clip-corner` |
| текст | `color`/`text-color`, `font` / `font-family` / `font-size` (наследуются), `text-align`, `letter-spacing`, `line-height`, `line-space`, `text-decoration`, `text-overflow`/`long-mode` (`ellipsis`, `scroll`, `clip`) |
| картинки | `image-fit`/`object-fit` (`stretch`/`fill`, `contain`, `cover`, `tile`, `center`), `image-recolor`/`image-color`, `image-recolor-opa`, `image-opa` |
| прочее | `opacity`/`opa`, `visibility`, `recolor`, `blend-mode`, `transition` (`<props> <time> <easing> <delay>`, `all`), `display` (`none`, `flex`), `layout`, `flex-flow`, `flex-direction`, `flex-wrap`, `justify-content`, `align-items`, `align-content`, `flex-grow`, `flex`, `overflow(-x/-y)`, `scrollbar-mode`, `position: absolute` (вне flex-потока), `pointer-events: none`, `cursor` (`pointer`, `text`, `move`, `not-allowed`… — показывает хост окна, напр. `CLvmlWin32Window`) |

Неизвестное свойство или значение — предупреждение через `CLvmlContext::SetLogger()`.

## Ресурсы

Пути относительно файла, который на них ссылается; `/x` — от корня ресурсов. Картинки читаются
через fs-драйвер LVGL на букве `L:` (`CLvmlFileSystem`) и декодируются lodepng (PNG). Шрифты TTF/OTF
рендерит tiny_ttf; размер — `font-size × scale`.

Нужно в `lv_conf.h`: `LV_USE_TINY_TTF`, `LV_USE_LODEPNG`, `LV_USE_SPAN` (для `richtext`),
`LV_USE_FLEX`, ненулевой `LV_CACHE_DEF_SIZE` (иначе PNG декодируется на каждом кадре); для
окна — `LV_USE_WINDOWS` + `LV_USE_OS LV_OS_WINDOWS`. Готовый вариант — `examples/lv_conf.h`.

## Сборка

```
cmake -S . -B build -G "MinGW Makefiles"     # или Visual Studio
cmake --build build
build/lvml_tests
```

В чужой проект (premake/MSBuild) достаточно добавить `source/*.cpp`, при необходимости
`platform/win32/*.cpp`, исходники LVGL и `LV_CONF_INCLUDE_SIMPLE` с путём к своему `lv_conf.h`.
LVGL 9.5 под MSVC: `osal/lv_windows.c` нужен `forceincludes { "stdlib.h" }`.

## html2lvml

```
html2lvml page.html out/ --name main
```

Пишет `out/main.lvml`, `out/main.lvss` и печатает отчёт: что не перенеслось (`::before/::after`,
`:not()`, `calc()`, grid, маски, фильтры…) и какие ассеты нужно дать в PNG (`.svg`, `.webp` → `.png`).
Что делается:

* теги сохраняются там, где lvml их знает, поэтому селекторы по тегам продолжают работать;
  `input`/`select`/`progress` → виджеты LVGL; `data-action="x"` → `onclick="x"`;
* обычный поток HTML выражается flex-классами: `.lv-col` (блоки столбиком), `.lv-row` (inline-элементы),
  `.lv-fill` (блок на всю ширину), `.lv-grow` (элемент `width: 100%` во flex-ряду — эмуляция `flex-shrink`);
* `left/right/top/bottom` → `x`/`y` + `align`; `background-size: cover|contain` → дочерний
  `<img class="lv-bg">` с `image-fit`;
* `html`, `body`, `:root` → класс `.lv-body` на `<screen>`.

Для SPA (React/Vue) нужен снимок отрисованного DOM, а не исходный `index.html`: стили dev-сервера Vite
лежат в `<style>` прямо в документе. Например, открыть страницу и выполнить в консоли
`'<!doctype html>\n' + document.documentElement.outerHTML`, сохранить в файл и подать его в
`html2lvml`. Логика (обработчики, данные, состояния) не конвертируется — её переносят на C++
поверх `CLvmlScene`.

## lvml-render

```
lvml-render <root> <scene.lvml> out.bmp [w h [scale [click-id|- [tree-depth]]]]
```

Рендерит сцену без окна, печатает предупреждения и события; `tree-depth` выводит дерево объектов
с координатами, флагом flex и выпадением из потока — удобно сверять вёрстку.
