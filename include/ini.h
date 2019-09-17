// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifndef __INI_H__
#define __INI_H__

#include <stdio.h>



struct inidescr_s;
struct iniinh_s;
struct iniparam_s;
struct inisect_s;
struct ini_s;

typedef void*(*fnIniMalloc)(size_t);
typedef void(*fnIniMallocTag)(unsigned);
typedef void(*fnIniFree)(void*);
typedef int(*fnIniFilter)(void*,void* userData);



#define INI_MTAG_STRING     0x01
#define INI_MTAG_DESCR      0x02
#define INI_MTAG_INHERIT    0x03
#define INI_MTAG_HEIR       0x04
#define INI_MTAG_PARAM      0x05
#define INI_MTAG_SECT       0x06



typedef struct {
    ptrdiff_t           length;     // Длинна строки
    ptrdiff_t           size;       // Размер выделенной памяти
    char                string[0];  // Сама строка
} inistring_t;

typedef struct inidescr_s {
    struct inidescr_s*  next;       // Следующий описатель файла
    struct ini_s*       ini;        // Указатель на ini
    inistring_t*        filename;   // Название ini файла
    struct inisect_s*   gsect;      // Глобальная секция в файле
    struct inisect_s*   lastSect;   // Последняя секция в файле
} inidescr_t;

typedef struct iniinh_s {
    struct iniinh_s*    next;       // Следующая унаследованная секция
    struct inisect_s*   inhSect;    // Указатель на унаследованную секцию
                                    // Или на наследника
    struct inisect_s*   sect;       // Указатель секции к которой наследуется
} iniinh_t;

typedef struct iniparam_s {
    struct iniparam_s*  next;       // Следующий параметр
    struct inisect_s*   sect;       // Указатель на секцию
    inistring_t*        key;        // Ключ параметра
    inistring_t*        value;      // Значение параметра
    inistring_t*        comment;    // Комментарий идущий после параметра
} iniparam_t;

typedef struct inisect_s {
    struct inisect_s*   next;       // Следующая секция
    struct inisect_s*   fnext;      // Следующая секция в этом файле
    inistring_t*        key;        // Название секции
    inistring_t*        comment;    // Комментарий идущий после секции
    iniparam_t*         firstParam; // Первый параметр в секции
    iniparam_t*         lastParam;  // Последний параметр в секции
    iniinh_t*           inherit;    // Список наследованных секций
    iniinh_t*           inheritLast;// Последний из списка наследованных секций
    iniinh_t*           heirs;      // Список секций прямых наследников
    iniinh_t*           heirsLast;  // Последний из списка прямых наследников
    inidescr_t*         filename;   // Файл в котором находится секция
} inisect_t;

typedef struct ini_s {
    fnIniMalloc         inimalloc;  // Функция аллокации памяти
    fnIniFree           inifree;    // Функция деаллокации памяти
    fnIniMallocTag      inimemtag;  // Функция задаёт тег аллокации памяти
    char*               errbuf;     // Буфер в который пишутся ошибки
    ptrdiff_t           errbufSize; // Размер буфера
    ptrdiff_t           bufFill;    // Сколько байт из буфера заполнено
    int                 numOfErrors;// Количество ошибок

    unsigned            flags;      // Флаги 0x1 - флаг буфера (используется
                                    //     внутренними функциями)
                                    // 0xff000000 - выравнивание ключа при 
                                    //     печати
    inisect_t*          firstSect;  // Первая секция в ini
    inisect_t*          lastSect;   // Последняя секция в ini
    inidescr_t*         filenames;  // Дескрипторы всех ini файлов
    inidescr_t*         lastfname;  // Последний дескриптор файла
} ini_t;

typedef struct {
    ini_t*              ini;        // Указатель на ini
    fnIniFilter         inifilter;  // Используемый callback фильтр
    void*               userData;   // Пользовательские данные
    inidescr_t*         descr;      // Файловый описатель
    inisect_t*          sect;       // Секция
    iniinh_t*           inh;        // Наследованного или наследник
    iniparam_t*         param;      // Параметр
    inistring_t*        string;     // ini-строка
    const char*         cstr;       // си строка
    ptrdiff_t           length;     // Длинна строки
} inihandler_t;



void IniInit( ini_t* ini, fnIniMalloc malloc, fnIniFree free, fnIniMallocTag memtag, char* buf, ptrdiff_t size );
// Инициализация ini структуры
// malloc - аллокатор памяти, вызывается во всех аллокациях памяти.
// free - освобождение памяти выделенной функцией malloc
// buf - указатель на буфер памяти, в который будут записываться ошибки
// парсинга, а size указывает на размер этой памяти.

void IniFree( ini_t* ini );
// Высвободить все ресурсы захваченные под ini структуру и вернуть всю память

void IniClearErrors( ini_t* ini );
// Очистить буфер ошибок



void IniSetParseComments( ini_t* ini, unsigned char flag );
// Парсить комментарии. Изначально установлено в 0
// Для включения парсинга нужно передать в flag значение 1
// При включении этой опции комментарии будут добавляться в секцию и на них
// будет выделяться дополнительная память

void IniSetKeyAlign( ini_t* ini, unsigned char align );
// Выравнивание ключа при печати. Изначально установлено в 0
// Для выравнивания ключа пробелами нужно передать в align значение большее за
// ноль. Если выравнивание окажется меньше чем длинна строки, то строка
// дописывается до конца

void IniSetEmptyLineAfterSection( ini_t* ini, unsigned char flag );
// Доавлять пустую строку в конец секции. Изначально установлено в 0
// Для добавления пустой строки в конце секции нужно передать в flag единицу.

void IniSetSpaceBeforeEqual( ini_t* ini, unsigned char flag );
// Печатать пробел перед знаком равенства. Изначально устоновлено в 0
// Для печати пробела между ключом и знаком равно нужно передать в flag 
// значение 1

void IniSetSpaceAfterEqual( ini_t* ini, unsigned char flag );
// Печатать пробел после знака равенства (только если keyalign == 0)
// Изначально установлено в 0

void IniSetPrintComments( ini_t* ini, unsigned char flag );
// Печатать комментарии (только если парсинг комментариев был включён)
// Изначально установлено в 0
// Если при парсинге файлов была включена опция парсинга комментариев, то
// комментарии можно будет вывести в файл

void IniSetPrintEmptyLines( ini_t* ini, unsigned char flag );
// Печатать пустые строки (если ключ, значение и комментарий пустые)
// Изначально установлено в 0
// Для печати пустых строк нужно передать в flag значение 1

void IniSetPrintFilenameInTop( ini_t* ini, unsigned char flag );
// Печатать название файла в виде комментария самом верху файла
// Изначально установлено в 0
// Для вывода названия файла вверху файла нужно передать в flags значение 1

void IniSetPrintFilenameInBottom( ini_t* ini, unsigned char flag );
// Печатать название файла в виде комментария в самом низу файла
// Изначально установлено в 0
// Для вывода названия файла внизу нужно передать в flags значение 1

void IniSetPrintHeirsBeforeSect( ini_t* ini, unsigned char flag );
// Печатать всех наследников перед секцией как комментарий
// Изначально установлено в 0
// Для вывода наследников нужно передать в flags значение 1

void IniSetCheckForSections( ini_t* ini, unsigned char flag );
// Проверять существование секций с таким же именем перед добавлением
// Изначально установлено в 1
// Для того что бы не было проверки на существование секций нужно передать
// в flag значение 0. Это может значительно ускоритьарсинг

void IniSetCheckForParameters( ini_t* ini, unsigned char flag );
// Проверять существование параметров с таким же именем перед добавлением вверху
// секцию
// Изначально установлено в 1
// Для выключения проверки существующих параметров нужно в flags передать 0
// Выключение проверки может ускорить парсинг



void IniExcludeParam( iniparam_t* param );
// Извлечь параметр param из секции и удалить его и все связанные с ним ссылки

void IniExcludeInherit( iniinh_t* inh );
// Извлечь унаследованную inh, и удалить inh из списка унаследованных,
// функция так же удаляет ссылку из наследников секций

void IniExcludeHeir( iniinh_t* inh );
// Извлечь наследника inh, и удалить inh из списка наследников, функция
// так же удаляет ссылку из унаследованных секций

void IniExcludeSect( inisect_t* sect );
// Извлечь секцию sect из списка секций, так же удаляет всех наследников



inidescr_t* IniAppendDescr( ini_t* ini, const char* filename );
// Добавить описатель файла (название файла) в ini
// Фукнция возвращает указатель на только что созданный дескриптор ini файла
// filename указывает на название создоваемого ini файла

inisect_t*  IniAppendSect( inidescr_t* descr, const char* key );
// Добавить секцию в файловый описатель (привязать секцию к конкретному файлу)
// Фукнция возвращает указатель на только что созданную секцию в 
// ini-дескрипторе. Секция имеет название key. В случае если секция с
// указанным именем уже существует фукнция ничего не предпринимает.

iniparam_t* IniAppendInclude( inidescr_t* descr, const char* filename );
// Добавить конструкцию #include "filename" в глобальную секцию
// Фукнция возвращает указатель на только что созданный параметр include
// filename - название включаемого файла

iniparam_t* IniAppendIncludeToSect( inisect_t* sect, const char* filename );
// Добавить конструкцию #include "filename" в секцию sect
// Фукнция возвращает указатель на только что созданный параметр include
// filename - название включаемого файла

iniparam_t* IniAppendParam( inisect_t* sect, const char* key, const char* val );
// Добавить параметр в секцию sect
// Фукнция возвращает указатель на только что созданный параметр с ключом key
// и значением val

iniparam_t* IniAppendComment( inisect_t* sect, const char* comment );
// Добавить комментарий comment в секцию sect
// Фукнция возвращает указатель на только что созданный параметр-комментарий



inisect_t*  IniFindSect( ini_t* ini, const char* key );
// Вернуть указатель на секцию с именем key (или NULL)
// Функция ищет секцию с названием key и возвращает указатель на эту секцию.
// В случае если найти указанную секцию не удалось возвращается указатель на
// NULL

iniparam_t* IniFindParam( inisect_t* sect, const char* key );
// Вернуть указатель на параметр с именем key из секции sect (или NULL)
// Фукнция ищет параметр с именем key в секции sect и возвращает указатель на
// эту секцию. В случае если такой парамер найти не удалось, то фукнция
// возвращает указатель на NULL

iniparam_t* IniFind( ini_t* ini, const char* sect, const char* param );
// Вернуть указатель на параметр param из секции sect (или NULL)
// Функция объединяет усилия двух функций IniFindSect и IniFindParam, и
// возвращает указатель на параметр, если удалось найти нужную секцию а в
// секции удалось найти нужный параметр.



int IniSectInherit( inisect_t* sect, const char* name );
// Унаследовать секцию с названием name, для секции sect
// Фукнция добавляет наследуемую секцию с именем name, для секции, на которую
// указывает sect.
// Фукнция возвращает -1 если не удалось найти секцию name.
// В случае успеха фукнция возвращает 0



int IniLoad( ini_t* ini, const char* filename );
// Загрузить ini из файла.
// Функция парсит ini-файл и включённые в него ini-файлы рекурсивно, и
// заполняет все данные. В случае ошибки открытия файла функция возвращает -1
// В противном случае возвращаемое значение будет равно 0
// Для проверки на наличие ошибок при парсинге файлов нужно смотреть список
// ошибок и количество ошибок парсинга

int IniSaveToFile( ini_t* ini, const char* filename );
// Сохранить всё ini содержимое в один файл с именем filename
// Функция возвращает 0 если удалось успешно сохранить ini в один файл.
// В случае ошибки возвращаемое значение будет равно -1

int IniSave( ini_t* ini );
// Обновить содержимое всех файлов
// Перезаписывает все ini-файлы которые были добавлены либо ранее распаршены.
// Функция возвращает 0 в случае успеха либо -1 в случае ошибки



/* Общее для inihandler_t*: (перебор данных)

Параметры:
filter - callback функция-фильтр, которая принимает на вход первым параметром
    фильтрующий элемент, а вторым userData. Если фильтр должен отбросить
    элемент, то функция должна вернуть 0. В других случаях функция возвращает
    отличное от нуля значение. Если в качестве filter передан NULL, то будут
    перебираться все данные по порядку их следования.
    
userData - пользовательские (произвольные) данные, которые передаются в фильтр
    filter вторым параметром. Имеет смысл только в том случае, если указан 
    параметр filter
    
  Все функции обхода данных возвращают указатель на данные (handler), handler 
всегда передаётся первым параметром в обработчик. Если фукнция вернула NULL,
то данных для обхода уже нет.

  Каждая из перебора функций изменяет свой набор данных в handler
*/

/* IniFirstFilename IniNextFilename
Результат выполнения IniFirstFilename и IniNextFilename:
    handler->descr - указатель на дескриптор файла
    handler->string - ini-строка с путём к файлу и названием файла
    handler->cstr - константный указатель на си строку
    handler->length - длинна строки
*/
void* IniFirstFilename( inihandler_t* handler, ini_t* ini, fnIniFilter filter, void* userData );
void* IniNextFilename( void* h );

/* IniFirstSect IniNextSect
Результат выполнения IniFirstSect и IniNextSect:
    handler->param - указатель на параметр
    handler->string - ini-строка, название текущего параметра (key)
    handler->cstr - константный указатель на си строку (key)
    handler->length - длинна строки (key)
*/
void* IniFirstSect( inihandler_t* handler, ini_t* ini, fnIniFilter filter, void* userData );
void* IniNextSect( void* h );

/* IniFirstSectInFile IniNextSectInFile
  Обход всех секций относящихся к одному файлу, начиная с глобальной секции.
(Имя глоальной секции не печатается)

Результат выполнения IniFirstSectInFile и IniNextSectInFile:
    handler->sect - указатель на наследованную секцию
    handler->inh - унаследованный элемент
    handler->string - ini-строка, название секции (key)
    handler->cstr - константный указатель на си строку (key)
    handler->length - длинна строки (key)
*/
void* IniFirstSectInFile( inihandler_t* handler, inidescr_t* descr, fnIniFilter filter, void* userData );
void* IniNextSectInFile( void* h );

/* IniFirstParam IniNextParam
  Обход параметров секции

Результат выполнения IniFirstInherit и IniNextInherit:
    handler->sect - указатель на секцию
    handler->string - ini-строка, название текущей секции (key)
    handler->cstr - константный указатель на си строку (key)
    handler->length - длинна строки (key)
*/
void* IniFirstParam( inihandler_t* handler, inisect_t* sect, fnIniFilter filter, void* userData );
void* IniNextParam( void* h );

/* IniFirstInherit IniNextInherit
  Обход унаследованных секций для указанной секции

Результат выполнения IniFirstInherit и IniNextInherit:
    handler->inh - указатель на наследованную секцию
    handler->sect - указатель на секцию
    handler->string - ini-строка, название текущей секции (key)
    handler->cstr - константный указатель на си строку (key)
    handler->length - длинна строки (key)
*/
void* IniFirstInherit( inihandler_t* handler, inisect_t* sect, fnIniFilter filter, void* userData );
void* IniNextInherit( void* h );

/* IniFirstHeir IniNextHeir
  Обход наследников секции

Результат выполнения IniFirstHeir и IniNextHeir:
    handler->inh - указатель на наследуемую секцию
    handler->sect - указатель на секцию
    handler->string - ini-строка, название текущей секции (key)
    handler->cstr - константный указатель на си строку (key)
    handler->length - длинна строки (key)
*/
void* IniFirstHeir( inihandler_t* handler, inisect_t* sect, fnIniFilter filter, void* userData );
void* IniNextHeir( void* h );



/* Общее для функций чтения (IniRead):
  Функция возвращает 0 - если чтение произошло без ошибок, и -1 если прочитать
данные не удалось. Первым параметром передаётся ini-параметр, из которого
нужно просканировать значение, вторым параметром передаётся указатлеь на
данные, в какую область памяти поместить значения
*/

int IniRead4fv( iniparam_t* param, float* fv );
// Читать вектор из четырёх чисел (п.з.): 1.0, 2.0, 3.0, 4.0

int IniRead3fv( iniparam_t* param, float* fv );
// Читать вектор из трёх чисел (п.з.): 1.0, 2.0, 3.0

int IniRead2fv( iniparam_t* param, float* fv );
//Читать вектор из двух чисел (п.з.): 1.0, 2.0

int IniRead1fv( iniparam_t* param, float* fv );
// Читать одно число с плавающей запятой: 1.0

int IniRead4iv( iniparam_t* param, int* iv );
//Читать четыре целых числа: 1, 2, 3, 4

int IniRead3iv( iniparam_t* param, int* iv );
// Читать три целых числа: 1, 2, 3

int IniRead2iv( iniparam_t* param, int* iv );
// Читать два целых числа: 1, 2

int IniRead1iv( iniparam_t* param, int* iv );
// Читать целое число: 1

int IniReadBool( iniparam_t* param, unsigned char* b );
// Читать bool значение true или false или целое число (переводится в 1 или 0)

int IniReadBoolv( iniparam_t* param, unsigned char* b, ptrdiff_t n );
// Читать последовательность из n логических значений, разделённых запятыми

int IniReadString( iniparam_t* param, char* s );
// Читать строку. Содержимое value копируется полностью в s, только в том
// случае если начало строки не начинается на ' или ". Если же строка
// начинается на ковычки, или двойные ковычки, то содержимое в этих ковычках
// будет обработано соответствующим образом, а так же будут обработаны все
// scape-последовательности.

int IniReadStringv( iniparam_t* param, char** s, ptrdiff_t n );
// То же самое что и IniReadString, но теперь сканируется последоватльность
// строк, разделённых запятыми, но только в том случае если перый символ
// это ' или "



#endif //__INI_H__