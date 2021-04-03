# Продвинутый ini-парсер
Описание фукнций можно посмотреть в файле **include/ini.h**.

# Основные структуры данных
```
inistring_t
inidescr_t
iniinh_t
iniparam_t
inisect_t
ini_t
inihandler_t
```

# Основные понятия

#### Файловый дескриптор
Файловый дескриптор это структура хранящая в себе список секций относящихся к одному файлу. При парсинге на каждый логический файл создаётся ровно один файловый дескриптор. 

#### Глобальная секция
Глобальная секция идёт с самого начала файла а заканчивается перед первым определением секции. Глобальная секция может в себе содержать только комментарии и директиву подключения файлов **#include "filename.ini"**

#### Секцияи
Секция начинается всегда с новой строки, содержит название и спиок унаследованных секций. Название секций заключенов **[]** квадратны скобки. Секции содержат в себе список параметров.

#### Наследование секций
Наследование секций идёт после знака **:** за закрывающей квадратной скобкой в названии секции. Список содержит названия секций, определённых выше по структуре файла, и разделяется запятыми между собой. Наследование ипользуется в случае поиска параметров. Если при поиске параметра в самой секции его не было найдено, то поисх переходит рекурсивно к унаследованным секциям.

#### Параметры секции
Параметр - это единица состовляющая секции. Параметр содержит в себе две логические единицы информации - это ключ и значение. Ключ отделён от значения символом **=** равно.

#### Комментарии
Комментарии могут быть в любой части файла. Комментарий начинается с символа **;** точка с запятой, а заканчивается символом **\n** концом строки.

### Пример содержимого ini-файла
```ini
; file "my_file.ini"
#include "new_file.ini"

;comment
[empty_section]; comment
; comment

[section_one]
key_sect1 = value1, value2, "string value\n"
key_sect2 = 1, 2, 3, 4; comment

[section_two]: section_one, empty_section; comment
key_sect1 = 123, 456
; comment
new_key = "Key value"; comment
```
