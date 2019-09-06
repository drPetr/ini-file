// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#ifndef __INI_H__
#define __INI_H__

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>



#define INI_USE_ASSERT



struct inidescr_s;
struct iniinh_s;
struct iniparam_s;
struct inisect_s;
struct ini_s;

typedef void*(*fnIniMalloc)(size_t);
typedef void(*fnIniFree)(void*);
typedef int(*fnIniFilter)(void*,void* userData);

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
    struct inisect_s*   sect;       // Указатель на унаследованную секцию
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
    iniinh_t*           inherited;  // Список прямых наследованных секций
    iniinh_t*           heirs;      // Список секций прямых наследников
    inidescr_t*         filename;   // Файл в котором находится секция
} inisect_t;

typedef struct ini_s {
    fnIniMalloc         inimalloc;  // Функция аллокации памяти
    fnIniFree           inifree;    // Функция деаллокации памяти
    char*               errbuf;     // Буфер в который пишутся ошибки
    ptrdiff_t           errbufSize; // Размер буфера
    ptrdiff_t           bufFill;    // Сколько байт из буфера заполнено
    int                 numOfErrors;// Количество ошибок
    
    unsigned            flags;      // Флаги 0x1 - флаг буфера (используется
                                    //     внутренними функциями)
                                    // 0x2 - флаг чтения (парсинга) 
                                    //     комментариев
                                    // 0x8000 - пустая линия после секции
                                    // 0x4000 - печатать ли комментарии
                                    // 0x2000 - пробел после знака =
                                    // 0x1000 - печатать пустые строки
                                    // 0x800 - пробел перед знаком = (только 
                                    //     если выравнивание равно нулю)
                                    // 0x400 - печатать имя текущего файла 
                                    //     в самом верху, перед печатью секций
                                    // 0x200 - печатать имя текущего файла 
                                    //     в самом низу, после всех секций
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



// Инициализация ini структуры
void IniInit( ini_t* ini, fnIniMalloc malloc, fnIniFree free, char* buf, ptrdiff_t size );
// Высвободить все ресурсы захваченные под ini структуру
void IniFree( ini_t* ini );
// Очистить список ошибок
void IniClearErrors( ini_t* ini );

// Парсить комментарии
void IniSetParseComments( ini_t* ini, unsigned char flag );
// Выравнивание ключа при печати
void IniSetKeyAlign( ini_t* ini, unsigned char align );
// Доавить пустую строку в конец секции
void IniSetEmptyLineAfterSection( ini_t* ini, unsigned char flag );
// Печатать пробел перед знаком равенства
void IniSetSpaceBeforeEqual( ini_t* ini, unsigned char flag );
// Печатать пробел после знака равенства (только если keyalign == 0)
void IniSetSpaceAfterEqual( ini_t* ini, unsigned char flag );
// Печатать комментарии (только если парсинг комментариев включён)
void IniSetPrintComments( ini_t* ini, unsigned char flag );
// Печатать пустые строки (если ключ, значение и комментарий пустые)
void IniSetPrintEmptyLines( ini_t* ini, unsigned char flag );
// Печатать название файла в виде комментария самом верху файла
void IniSetPrintFilenameInTop( ini_t* ini, unsigned char flag );
// Печатать название файла в виде комментария в самом низу файла
void IniSetPrintFilenameInBottom( ini_t* ini, unsigned char flag );

// Добавить описатель файла (название файла) в ini
inidescr_t* IniAppendDescr( ini_t* ini, const char* filename );
// Добавить секцию в файловый описатель (привязать секцию к конкретному файлу)
inisect_t*  IniAppendSect( inidescr_t* descr, const char* key );
// Добавить конструкцию #include "path" в глобальную секцию описателя файла
iniparam_t* IniAppendInclude( inidescr_t* descr, const char* filename );
// Добавить конструкцию #include "path" в указанную секцию описателя файла
iniparam_t* IniAppendIncludeToSect( inisect_t* sect, const char* filename );
// Добавить параметр в указанную секцию
iniparam_t* IniAppendParam( inisect_t* sect, const char* key, const char* val );
// Добавить комментарий в указанную секцию
iniparam_t* IniAppendComment( inisect_t* sect, const char* comment );

// Вернуть указатель на секцию с именем key (или NULL)
inisect_t*  IniFindSect( ini_t* ini, const char* key );
// Вернуть указатель на параметр с именем key из секции sect (или NULL)
iniparam_t* IniFindParam( inisect_t* sect, const char* key );
// Вернуть указатель на параметр param из секции sect (или NULL)
iniparam_t* IniFind( ini_t* ini, const char* sect, const char* param );

// Унаследовать секцию с названием name, для секции sect
int IniSectInherit( inisect_t* sect, const char* name );

// Загрузить из файла
int IniLoad( ini_t* ini, const char* filename );
// Сохранить всё ini содержимое в один файл с именем filename
int IniSaveToFile( ini_t* ini, const char* filename );
// Обновить содержимое всех файлов
int IniSave( ini_t* ini );




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

// Читать вектор из четырёх чисел (п.з.): 1.0, 2.0, 3.0, 4.0
int IniRead4fv( iniparam_t* param, float* fv );
// Читать вектор из трёх чисел (п.з.): 1.0, 2.0, 3.0
int IniRead3fv( iniparam_t* param, float* fv );
//Читать вектор из двух чисел (п.з.): 1.0, 2.0
int IniRead2fv( iniparam_t* param, float* fv );
// Читать одно число с плавающей запятой: 1.0
int IniRead1fv( iniparam_t* param, float* fv );
//Читать четыре целых числа: 1, 2, 3, 4
int IniRead4iv( iniparam_t* param, int* iv );
// Читать три целых числа: 1, 2, 3
int IniRead3iv( iniparam_t* param, int* iv );
// Читать два целых числа: 1, 2
int IniRead2iv( iniparam_t* param, int* iv );
// Читать целое число: 1
int IniRead1iv( iniparam_t* param, int* iv );
// Читать bool значение true или false или целое число (переводится в 1 или 0)
int IniReadBool( iniparam_t* param, unsigned char* b );
// Читать последовательность из n логических значений, разделённых запятыми
int IniReadBoolv( iniparam_t* param, unsigned char* b, ptrdiff_t n );

/*
  Читать строку. Содержимое value копируется полностью в s, только в том
случае если начало строки не начинается на ' или ". Если же строка начинается
на ковычки, или двойные ковычки, то содержимое в этих ковычках будет
обработано соответствующим образом, а так же будут обработаны все scape-
последовательности.
*/
int IniReadString( iniparam_t* param, char* s );

/*
  То же самое что и выше, но теперь сканируется последоватльность строк,
разделённых запятыми, но только в том случае если перый символ это ' или "
*/
int IniReadStringv( iniparam_t* param, char** s, ptrdiff_t n );

#endif //__INI_H__