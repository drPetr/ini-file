// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ini.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#ifdef ININO_DEBUG
    #define iniassert(expr)
#else
    #define iniassert(expr) assert(expr)
#endif


typedef struct {
    const char*     word;
    ptrdiff_t       length;
} inikeyword_t;

typedef struct {
    char*       begin;
    char*       forward;
    ptrdiff_t   length;
    int         token;
} iniscan_t;



const inikeyword_t inikeywords[] = {
    { "include", 7 },
    { "print", 5 },
    { "true", 4 },
    { "false", 5 },
    { NULL, 0 }
};



/*
================
IniStringCreate
================
*/
static inistring_t* IniStringCreate( ini_t* ini, const char* str, ptrdiff_t len ) {
    inistring_t* s;
    
    iniassert( ini );
    
    if( !str || len == 0 ) {
        return NULL;
    }
    if( len < 0 ) {
        len = strlen(str);
    }
    s = (inistring_t*)ini->inimalloc( sizeof(inistring_t) + len + 1 );
    s->size = len + 1;
    s->length = len;
    strncpy( s->string, str, len );
    s->string[len] = 0;
    return s;
}

/*
================
IniInheritCreate
================
*/
static iniinh_t* IniInheritCreate( ini_t* ini, inisect_t* sect ) {
    iniinh_t* inh;
    
    iniassert( ini );
    iniassert( sect );
    
    inh = (iniinh_t*)ini->inimalloc( sizeof(iniinh_t) );
    inh->next = NULL;
    inh->sect = sect;
    return inh;
}

/*
================
IniHeirCreate
================
*/
static iniinh_t* IniHeirCreate( ini_t* ini, inisect_t* sect ) {
    /*iniinh_t* inh;
    
    iniassert( ini );
    iniassert( sect );
    
    inh = (iniinh_t*)ini->inimalloc( sizeof(iniinh_t) );
    inh->next = NULL;
    inh->sect = sect;
    return inh;*/
    return IniInheritCreate( ini, sect );
}

/*
================
IniParamCreate
================
*/
static iniparam_t* IniParamCreate( ini_t* ini, inistring_t* key, inistring_t* value, inistring_t* comment ) {
    iniparam_t* p;
    
    iniassert( ini );
    
    p = (iniparam_t*)ini->inimalloc( sizeof(iniparam_t) );
    p->next = NULL;
    p->sect = NULL;
    p->key = key;
    p->value = value;
    p->comment = comment;
    return p;
}

/*
================
IniSectCreate
================
*/
inisect_t* IniSectCreate( ini_t* ini, inistring_t* key, inistring_t* comment ) {
    inisect_t* s;
    
    iniassert( ini );
    iniassert( key );
    
    s = (inisect_t*)ini->inimalloc( sizeof(inisect_t) );
    s->next = NULL;
    s->fnext = NULL;
    s->key = key;
    s->comment = comment;
    s->firstParam = NULL;
    s->lastParam = NULL;
    s->inherited = NULL;
    s->heirs = NULL;
    s->filename = NULL;
    return s;
}

/*
================
IniDescrCreate
================
*/
static inidescr_t* IniDescrCreate( ini_t* ini, const char* filename, ptrdiff_t len ) {
    inidescr_t* d;
    
    iniassert( ini );
    iniassert( filename );
    
    d = (inidescr_t*)ini->inimalloc( sizeof(inidescr_t) );
    d->next = NULL;
    d->ini = ini;
    d->filename = IniStringCreate( ini, filename, len );
    d->gsect = IniSectCreate( ini,
        IniStringCreate( ini, "_g", -1 ),
        NULL
    );
    d->lastSect = d->gsect;
    d->gsect->filename = d;
    return d;
}

/*
================
IniAppendDescr_s
================
*/
static void IniAppendDescr_s( ini_t* ini, inidescr_t* descr ) {
    iniassert( ini );
    iniassert( descr );
    iniassert( !(ini->filenames) == !(ini->lastfname) );
    
    if( ini->filenames ) {
        ini->lastfname->next = descr;
        ini->lastfname = descr;
    } else {
        ini->filenames = descr;
        ini->lastfname = descr;
    }
}

/*
================
IniAppendSect_s
================
*/
static void IniAppendSect_s( inidescr_t* descr, inisect_t* sect ) {
    ini_t* ini;
    
    iniassert( descr );
    iniassert( sect );
    iniassert( descr->ini );
    iniassert( !sect->filename );
    
    ini = descr->ini;
    if( ini->firstSect ) {
        ini->lastSect->next = sect;
        ini->lastSect = sect;
    } else {
        ini->firstSect = sect;
        ini->lastSect = sect;
    }
    descr->lastSect->fnext = sect;
    descr->lastSect = sect;
    sect->filename = descr;
}

/*
================
IniAppendParam_s
================
*/
static void IniAppendParam_s( inisect_t* sect, iniparam_t* param ) {
    iniassert( sect );
    iniassert( param );
    iniassert( !(sect->firstParam) == !(sect->lastParam) );
    
    param->sect = sect;
    if( sect->firstParam ) {
        sect->lastParam->next = param;
        sect->lastParam = param;
    } else {
        sect->firstParam = param;
        sect->lastParam = param;
    }
}

/*
================
IniFindOnlyInSect
================
*/
static iniparam_t* IniFindOnlyInSect( inisect_t* sect, const char* key, ptrdiff_t len ) {
    iniparam_t* p;
    
    iniassert( sect );
    
    p = sect->firstParam;
    while( p ) {
        if( len == p->key->length && !strncmp(key, p->key->string, len) ) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

/*
================
IniFindInInherit
================
*/
static iniparam_t* IniFindInInherit( inisect_t* sect, const char* key, ptrdiff_t length ) {
    iniinh_t* inh;
    iniparam_t* p;;
    
    iniassert( sect );
    
    inh = sect->inherited;
    while( inh ) {
        p = IniFindOnlyInSect( inh->sect, key, length );
        if( p ) {
            return p;
        }
        p = IniFindInInherit( inh->sect, key, length );
        if( p ) {
            return p;
        }
        inh = inh->next;
    }
    
    return NULL;
}

/*
================
IniFiledescrFind
================
*/
static inidescr_t* IniFiledescrFind( ini_t* ini, const char* filename, ptrdiff_t len ) {
    inidescr_t* it;
    const char* s1;
    const char* s2;
    
    iniassert( ini );
    iniassert( filename );
    
    if( len < 0 ) {
        len = strlen( filename );
    }
    
    it = ini->filenames;
    while( it ) {
        if( it->filename->length == len ) {
            s1 = filename;
            s2 = it->filename->string;
            while( *s1 ) {
                if( !(((*s1 == '\\' || *s1 == '/') && 
                    (*s2 == '\\' || *s2 == '/')) || 
                    (tolower(*s1) == tolower(*s2))) ) {
                    break;
                }
                s1++;
                s2++;
            }
            if( *s1 == 0 ) {
                return it;
            }
        }
        it = it->next;
    }
    
    return NULL;
}

/*
================
IniPrint
================
*/
static void IniPrint( ini_t* ini, const char* fmt, ... ) {
    va_list args;
    
    if( ini->errbuf ) {
        va_start( args, fmt );
        if( (ini->errbufSize - ini->bufFill) > 128 ) {
            ini->bufFill += vsprintf( ini->errbuf + ini->bufFill, fmt, args );
        } else if( (ini->flags & 0x1) == 0 && (ini->errbufSize - ini->bufFill) > 6 ) {
            ini->bufFill += vsprintf( ini->errbuf + ini->bufFill, "...\n", args );
            ini->flags |= 0x1;
        }
        va_end( args );
    }
    ini->numOfErrors++;
}


/*
================
IniScanToken
================
*/
#define INI_IDENTIFICATOR   0x1f00  // identificator
#define INI_EQUAL           '='     // string
#define INI_SECT_OPEN       '['     // identificator
#define INI_SECT_CLOSE      ']'     //
#define INI_COMMA           ','     // identificator
#define INI_COMMENT         ';'     // comment
#define INI_INHERIT         ':'     // identificator
#define INI_PREPROCESSOR    '#'     // string
#define INI_INCLUDE_PATH    '\"'    // "string"

#define f   (s->forward)
#define b   (s->begin)
#define l   (s->length)
#define tk  (s->token)

static int IniIsSpace( int ch ) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\v' || 
        ch == '\f' || ch == '\r';
}

static void IniSkipSpaces( char** p ) {
    while( IniIsSpace(**p) ) {
        (*p)++;
    }
}

static int IniIsIdSymbol( int ch ) {
    return ch == '_' || ch == '$' || ch == '-' || ch == '.' || 
        ch == ':' || ch == '\\' || ch == '#' || ch == '(' || ch == ')'
        || ch == ',' || ch == '?' || ch == '@';
}

static int IniIsIdSymbol2( int ch ) {
    return ch == '_' || ch == '$' || ch == '-' || ch == '.' || 
        ch == ':' || ch == '\\' || ch == '#' || ch == '(' || ch == ')'
        || ch == '?' || ch == '@';
}

static int IniScanIdentificator( iniscan_t* s ) {
    IniSkipSpaces( &f );
    b = f;
    while( isalnum(*f) || IniIsIdSymbol(*f) ) {
        f++;
    }
    l = f - b;
    return l ? 0 : -1;
}

static int IniScanIdentificator2( iniscan_t* s ) {
    IniSkipSpaces( &f );
    b = f;
    while( isalnum(*f) || IniIsIdSymbol2(*f) ) {
        f++;
    }
    l = f - b;
    return l ? 0 : -1;
}

static int IniScanValue( iniscan_t* s ) {
    IniSkipSpaces( &f );
    b = f;
    while( *f && !(*f == ';' || *f == '/' || *f == '\n') ) {
        if( !IniIsSpace(*f) ) {
            l = f - b;
        }
        f++;
    }
    if( b != f && (*f == ';' || *f == '/' || *f == '\n') ) {
        l++;
    }
    return 0;
}

static int IniScanComment( iniscan_t* s ) {
    b = f;
    l = 0;
    while( *f && *f != '\n' ) {
        if( !IniIsSpace(*f) ) {
            l = f - b + 1;
        }
        f++;
    }
    return 0;
}

static int IniScanIncludePath( iniscan_t* s ) {
    char ch = *f;
    f++;
    b = f;
    while( *f && *f != ch ) {
        f++;
    }
    l = f - b;
    if( *f && *f == ch ) {
        f++;
        return 0;
    } else {
        return -1;
    }
}

static int IniFindKeyword( const char* word, ptrdiff_t len ) {
    int i;
    for( i = 0; inikeywords[i].word; i++ ) {
        if( len == inikeywords[i].length && 
            !strncmp( inikeywords[i].word, word, len ) ) {
            return i;
        }
    }
    return -1;
}

static int IniScanToken( iniscan_t* s ) {
    l = 0;

    for(;;) {
        b = f;
        switch( *f ) {
            case 0: tk = 0; l = 0; return 0;
            case ' ': case '\t': case '\n': case '\v': case '\f':
            case '\r': f++; break;
            case '=':
                f++;
                IniScanValue( s );
                return tk = INI_EQUAL;
            case '[':
                f++;
                if( IniScanIdentificator( s ) ) {
                    return tk = -1;
                }
                return tk = INI_SECT_OPEN;
            case ']':
                f++;
                l = 1;
                return tk = INI_SECT_CLOSE;
            case ',':
                f++;
                if( IniScanIdentificator2( s ) ) {
                    return tk = -1;
                }
                return tk = INI_COMMA;
            case ';': case '/':
                f++;
                IniScanComment( s );
                return tk = INI_COMMENT;
            case ':':
                f++;
                if( IniScanIdentificator2( s ) ) {
                    return tk = -1;
                }
                return tk = INI_INHERIT;
            case '#':
                f++;
                if( IniScanIdentificator( s ) ) {
                    return tk = -1;
                }
                return tk = INI_PREPROCESSOR;
            case '\'': case '"':
                if( IniScanIncludePath( s ) ) {
                    return tk = -1;
                }
                return tk = INI_INCLUDE_PATH;
            default:
                if( IniScanIdentificator( s ) ) {
                    return tk = -1;
                }
                return tk = INI_IDENTIFICATOR;
        }
    }
}

/*
================
IniPathCopy

Скопировать путь к файлу из src в dst, функция возвращает указатель на символ
следующий после конца пути к файлу.

src: "path\to\file\filename.ext"
dst: "path\to\file\"
return:            ^ - pointer to this NULL symbol
================
*/
static char* IniPathCopy( char* dst, const char* src ) {
    char* ret = dst;
    ptrdiff_t i = 0;
    for( ; src[i]; i++ ) {
        if( src[i] == '\\' || src[i] == '/' ) {
            ret = dst + i + 1;
        }
    }
    strncpy( dst, src, ret - dst );
    *ret = 0;
    return ret;
}

/*
================
IniRecursiveParse
================
*/
static int IniRecursiveParse( ini_t* ini, const char* filename, ptrdiff_t relative ) {
    iniparam_t* param;      // Pointer to parameter
    inidescr_t* descr;      // Pointer to description
    iniscan_t* s;           // Scanner pointer
    inisect_t* sect;        // Current section
    iniscan_t scan;         // Scanner
    FILE* file;             // Current file
    char* key;              // Key pointer
    char* val;              // Value pointer
    char* nextfname;        // Pointer to filename
    char buf[4096*2];       // Scanner buffer
    char nextpath[1024];    // Path to file
    char string[1024];      // Used for inheritance
    ptrdiff_t keylen;       // Key length
    ptrdiff_t vallen;       // Value length
    int line;               // Current line in the file
    int ret;                // Return code
    int tmp;                //
    
    s = &scan;
    param = NULL;
    nextfname = NULL;
    line = 0;
    ret = 0;
    //scan.forward = buf;
    
    // Open current file
    if( (file = fopen( filename, "r" )) == NULL ) {
        IniPrint( ini, "error: can not open file '%s'\n", filename );
        return -1;
    }
    
    // Append current filename to filedescr
    descr = IniAppendDescr( ini, filename );
    sect = descr->gsect;
    // Main parsing loop
    while( fgets( buf, sizeof(buf), file ) != NULL ) {
        line++;
        f = buf; //-V507

        switch( IniScanToken( s ) ) {
            case INI_IDENTIFICATOR:
                // Save pointer to key and key length
                key = b;
                keylen = l;
                
                IniScanToken( s );
                // Save pointer to value and value length (if token is value)
                if( tk == INI_EQUAL ) {
                    val = b;
                    vallen = l;
                    IniScanToken( s );
                } else {
                    val = NULL;
                    vallen = 0;
                }
                
                // Check section. Section cannot be is global
                if( sect != descr->gsect ) {
                    // Append parametr to section
                    param = IniParamCreate( ini,
                        IniStringCreate( ini, key, keylen ),
                        IniStringCreate( ini, val, vallen ),
                        NULL
                    );
                    IniAppendParam_s( sect, param );
                } else {
                    IniPrint( ini, "error: section start expected line:%d \
file:'%s'\n", line, filename );
                    ret = -1;
                    continue;
                }
                
                // Append comment to current sectoin (if token is comment)
                if( ini->flags & 0x2 && tk == INI_COMMENT ) {
                    if( !param->comment ) {
                        param->comment = IniStringCreate( ini, b, l );
                    }
                    // Scan next token
                    IniScanToken( s );
                }
                
                break;
                
            // Parse next sequence:
            // [section]: inherit1, inherit2, ... , inherit_n ; comment
            case INI_SECT_OPEN:
                key = b;
                keylen = l;
                
                IniScanToken( s );
                // Expect close section symbol ']'
                if( tk != INI_SECT_CLOSE ) {
                    IniPrint( ini, "error: expected ']' line:%d file'%s'\n", 
                        line, filename );
                    ret = -1;
                    continue;
                }
                
                // Create new section and append section to filedescr
                sect = IniSectCreate( ini,
                    IniStringCreate( ini, key, keylen ),
                    NULL
                );
                IniAppendSect_s( descr, sect );
                
                IniScanToken( s );
                // Check for inherit and parse 'inherit' sequences
                while( tk == INI_COMMA || tk == INI_INHERIT ) {
                    strncpy( string, b, l );
                    string[l] = 0;
                    // Inherit for current section
                    if( IniSectInherit( sect, string ) ) {
                        IniPrint( ini, "error: can not find section for \
inherit '%s' line:%d file:'%s'\n", string, line, filename );
                        ret = -1;
                    }
                    // Scan next token
                    IniScanToken( s );
                }
                
                // Append comment to current sectoin (if token is comment)
                if( !!(ini->flags & 0x2) && tk == INI_COMMENT ) {
                    if( !sect->comment ) {
                        sect->comment = IniStringCreate( ini, b, l );
                    }
                    // Scan next token
                    IniScanToken( s );
                }
                
                break;
            
            // Parse next sequence:
            // #preproc "path\filename.ext" ; comment
            case INI_PREPROCESSOR:
                switch( IniFindKeyword( b, l ) ) {
                    
                    // Parse next sequence:
                    // #include "path\filename.ext" ; comment
                    case 0:
                        IniScanToken( s );
                        if( tk == INI_INCLUDE_PATH ) {
                            
                            // If new file path is not initialized
                            if( nextfname == NULL ) {
                                nextfname = IniPathCopy( nextpath, filename );
                                relative = nextfname - nextpath;
                            }
                            
                            // Append new filename to new file path
                            strncpy( nextfname, b, l );
                            nextfname[l] = 0;
                            
                            // Check the included file for already include
                            if( IniFiledescrFind( ini, nextpath, -1 ) ) {
                                IniPrint( ini, "warning: file '%s' is \
already included line:%d file:'%s'\n", nextpath, line, filename );
                                continue;
                            } else {
                                // Append parametr to section
                                param = IniAppendIncludeToSect( sect, 
                                    nextpath + relative
                                );
                                // Parsing nested include files
                                tmp = IniRecursiveParse( ini, nextpath, 
                                    relative
                                );
                                ret = ret ? tmp : ret;
                            }
                        } else {
                            IniPrint( ini, "error: expected included file \
name line:%d file:'%s'\n", line, filename );
                            ret = -1;
                            continue;
                        }
                        break;
                        
                    // Parse next sequence:
                    // #print "to print" ; comment
                    case 1:
                        IniScanToken( s );
                        if( tk == INI_INCLUDE_PATH ) {
                            // Create new parameter
                            param = IniParamCreate( ini,
                                IniStringCreate( ini, "#print", 6 ),
                                IniStringCreate( ini, b, l ),
                                NULL
                            );
                            // Append to section
                            IniAppendParam_s( sect, param );
                            // And print data to stdout
                            fprintf( stdout, "%.*s\n", (int)l, b );
                        } else {
                            IniPrint( ini, "error: expected printing value \
line:%d file:'%s'\n", line, filename );
                            ret = -1;
                            continue;
                        }
                        break;
                        
                    // Uncnown #keyword
                    default:
                        IniPrint( ini, "error: uncnown directive '%.*s' \
line:%d file:'%s'\n", (int)l, b, line, filename );
                        ret = -1;
                        continue;
                }
                IniScanToken( s );
                
                // Append comment to current parametr
                if( !!(ini->flags & 0x2) && tk == INI_COMMENT ) {
                    if( !param->comment ) {
                        param->comment = IniStringCreate( ini, b, l );
                    }
                    IniScanToken( s );
                }
                break;
                
            // Parse next sequence:
            // ; comment
            case INI_COMMENT:
                if( !!(ini->flags & 0x2) ) {
                    // Create comment
                    param = IniParamCreate( ini, NULL, NULL,
                        IniStringCreate( ini, b, l )
                    );
                    if( sect != NULL ) {
                        // Append comment to current section
                        IniAppendParam_s( sect, param );
                    } else {
                        // Append comment to global section
                        IniAppendParam_s( descr->gsect, param );
                    }
                }
                // Scan next token and break from case
                IniScanToken( s );
                break;
        }
        
        // Check for next empty token
        if( !(tk == 0 || (tk == INI_COMMENT && !(ini->flags & 0x2))) ) { 
            IniPrint( ini, "error: uncnown token '%.*s' line:%d file:'%s'\n", 
                (int)l, b, line, filename );
            ret = -1;
        }
    }
    
    // Check if the file is read correctly
    if( !feof(file) && ferror(file) ) {
        IniPrint( ini, "error: error reading file '%s'\n", filename );
        ret = -1;
    }
    fclose(file);
    
    return ret;
}

#undef f
#undef b
#undef l
#undef tk

/*
================
IniFprintSect
================
*/
static void IniFprintSect( FILE* f, inisect_t* s, int includeignore ) {
    unsigned flags;         // Флаги для печати
    int keyalign;           // Выравнивание ключа (выравнивание пробелами)
    int emptyline;          // Печатать ли пустую строку после конца секции
    int printcomment;       // Печатать ли комментарии
    int spaceaftereq;       // Пробел после знака равно
    int printempty;         // Печатать пустые строки, если такие есть
    int spacebeforeeq;      // Печатать пробел перед знаком 
    iniinh_t* inh;          // Унаследованные секции
    iniparam_t* p;          // Параметры секции

    iniassert( f );
    iniassert( s );
    iniassert( s->filename );
    iniassert( s->filename->gsect );

    flags = s->filename->ini->flags;
    keyalign = (flags >> 24) & 0xff;
    emptyline = flags & 0x8000;
    printcomment = flags & 0x4000;
    spaceaftereq = flags & 0x2000;
    printempty = flags & 0x1000;
    spacebeforeeq = keyalign == 0 && (flags & 0x800);

    // [section_name]:inherit1, inherit2, ...; comment
    if( s != s->filename->gsect ) {
        if( s->key ) {  // [section_name]
            fprintf( f, "[%s]", s->key->string );
        }
        inh = s->inherited;
        if( inh ) {     // :inherit1, inherit2, ...
            fprintf( f, ":" );
            while( inh ) {
                fprintf( f, "%s", inh->sect->key->string );
                inh = inh->next;
                if( inh ) {
                    fprintf( f, ", " );
                }
            }
        }
        if( printcomment && s->comment ) {  // ; comment
            fprintf( f,";%s\n", s->comment->string );
        } else {
            fprintf( f, "\n" );
        }
    }

    // key = value; comment
    p = s->firstParam;
    while( p ) {
        if( p->key && p->key->string[0] == '#' ) {
            // #preproc_key "value"; comment
            if( includeignore && 
                !strncmp( p->key->string, "#include", 8 ) ) {
                goto goIgnore;
            }
            fprintf( f, "%s", p->key->string );
            if( p->value ) {
                fprintf( f, " \"%s\"", p->value->string );
            }
            goIgnore:
            if( printcomment && p->comment ) {
                fprintf( f, ";%s", p->comment->string );
            }
        } else {
            // key = value; comment
            if( p->key ) {
                if( p->value || (printcomment && p->comment) ) {
                    fprintf( f, "%-*s", keyalign, p->key->string );
                } else {
                    fprintf( f, "%s", p->key->string );
                }
            }
            if( p->value ) {
                if( spacebeforeeq && spaceaftereq ) {
                    fprintf( f, " = %s", p->value->string );
                } else if( spacebeforeeq ) {
                    fprintf( f, " =%s", p->value->string );
                } else if( spaceaftereq ) {
                    fprintf( f, "= %s", p->value->string );
                } else {
                    fprintf( f, "=%s", p->value->string );
                }
            }
            if( p->comment && printcomment ) {
                fprintf( f, ";%s", p->comment->string );
            }
        }
        if( ((p->comment && printcomment) || p->key || p->value) &&
            !(p->key && p->key->string[0] == '#' && includeignore) ) {
            fprintf( f, "\n" );
        } else if( printempty ) {
            fprintf( f, "\n" );
        }
        p = p->next;
    }
        
    // empty line after section
    if( emptyline ) {
        fprintf( f, "\n" );
    }
}

/*
================
IniFprintFiledescr
================
*/
static void IniFprintFiledescr( FILE* f, inidescr_t* d, int includeignore ) {
    unsigned flags;
    inisect_t* s;
    
    iniassert( f );
    iniassert( d );
    iniassert( d->ini );
    
    flags = d->ini->flags;
    // print the file name at the top of the file
    if( flags & 0x400 ) {
        fprintf( f, "; %s\n", d->filename->string );
    }
    
    s = d->gsect;
    while( s ) {
        IniFprintSect( f, s, includeignore );
        s = s->fnext;
    }
    
    // print the file name at the bottom of the file
    if( flags & 0x200 ) {
        fprintf( f, "; %s\n", d->filename->string );
    }
}

/*
================
IniFprint
================
*/
static void IniFprint( FILE* f, ini_t* ini, int includeignore ) {
    inisect_t* s;
    
    iniassert( f );
    iniassert( ini );
    
    s = ini->firstSect;
    while( s ) {
        IniFprintSect( f, s, includeignore );
        s = s->next;
    }
}

/*
================
IniFreeSect
================
*/
static void IniFreeSect( inisect_t* s ) {
    fnIniFree free;
    iniparam_t* p;
    iniparam_t* ptmp;
    iniinh_t* inh;
    iniinh_t* inhtmp;
    iniinh_t* heir;
    iniinh_t* heirtmp;
    
    iniassert( s );
    iniassert( s->filename );
    iniassert( s->filename->ini );
    
    free = s->filename->ini->inifree;
    p = s->firstParam;
    inh = s->inherited;
    heir = s->heirs;
    
    // free sect key
    if( s->key ) {
        free( s->key );
    }
    // free sect comment
    if( s->comment ) {
        free( s->comment );
    }
    // free sect parametr
    while( p ) {
        // free parametr key
        if( p->key ) {
            free( p->key );
        }
        // free parametr value
        if( p->value ) {
            free( p->value );
        }
        // free parametr comment
        if( p->comment ) {
            free( p->comment );
        }
            
        ptmp = p;
        p = p->next;
        free(ptmp);
    }
    // free inherit
    while( inh ) {
        inhtmp = inh;
        inh = inh->next;
        free(inhtmp);
    }
    // free heirs
    while( heir ) {
        heirtmp = heir;
        heir = heir->next;
        free(heirtmp);
    }
    // free section
    free(s);
}

/*
================
IniScanBool
================
*/
static int IniScanBool( char* str, char** endptr, unsigned char* b ) {
    const struct { 
        const char* val;
        ptrdiff_t len;
    } t[] = {
        {"true", 4},    // 0
        {"false", 5},   // 1
        {"on", 2},      // 2
        {"off", 3},     // 3
        {NULL, 0}       // 4
    };
    
    int i;
    int v;
    IniSkipSpaces( &str );
    
    if( isdigit(*str) ) {
        // If is digit value (decimal)
        if( sscanf( str, "%d", &v ) != 1 ) {
            return -1;
        }
        while( isdigit(*str) || *str == '-' ) {
            str++;
        }
        if( *str == 0 || IniIsSpace(*str) || *str == ',' ) {
            if( endptr ) {
                *endptr = str;
            }
            *b = !!v;
            return 0;
        }
    } else {
        // If value is string
        for( i = 0; t[i].val; i++ ) {
            if( !strncmp( str, t[i].val, t[i].len ) ) {
                str += t[i].len;
                if( *str == 0 || IniIsSpace(*str) || *str == ',' ) {
                    if( endptr ) {
                        *endptr = str;
                    }
                    *b = !(i & 1);
                    return 0;
                }
                return -1;
            }
        }
    }
    
    return -1;
}

/*
================
IniScanString
================
*/
static int IniScanString( char* src, char** endptr, char* dst ) {
    char c;
    
    IniSkipSpaces( &src );
    c = *src;
    if( c == 0 ) {
        return -1;
    }
    // Escape sequences processed and can be string list separate of comma
    if( c == '\'' || c == '\"' ) {
        src++;
        while( *src && *src != c ) {
            if( *src == '\\' ) {
                src++;
                switch( *src++ ) {
                    case '\'': *dst++ = '\''; break;
                    case '"': *dst++ = '\"'; break;
                    case '?': *dst++ = '\?'; break;
                    case '\\': *dst++ = '\\'; break;
                    case '0': *dst++ = '\0'; break;
                    case 'b': *dst++ = '\b'; break;
                    case 'f': *dst++ = '\f'; break;
                    case 'n': *dst++ = '\n'; break;
                    case 'r': *dst++ = '\r'; break;
                    case 't': *dst++ = '\t'; break;
                    case 'v': *dst++ = '\v'; break;
                    default: *dst++ = *src; break;
                }
            } else {
                *dst = *src;
            }
        }
        if( *src == c ) {
            src++;
        }
    } else {
        while( *src ) {
            *dst++ = *src++;
        }
        *dst = 0;
    }
    if( endptr ) {
        *endptr = src;
    }
    return 0;
}



/*
================================================================

                    definition of functions
                    
================================================================
*/

/*
================
IniInit
================
*/
void IniInit( ini_t* ini, fnIniMalloc malloc, fnIniFree free, char* buf, ptrdiff_t size ) {
    iniassert( ini );
    iniassert( malloc );
    iniassert( free );
    
    ini->inimalloc = malloc;
    ini->inifree = free;
    ini->errbuf = buf;
    ini->errbufSize = size;
    ini->numOfErrors = 0;
    ini->bufFill = 0;
    ini->flags = 0;
    ini->firstSect = NULL;
    ini->lastSect = NULL;
    ini->filenames = NULL;
    ini->lastfname = NULL;
}

/*
================
IniFree
================
*/
void IniFree( ini_t* ini ) {
    fnIniFree free;
    inisect_t* s;
    inisect_t* stmp;
    inidescr_t* d;
    inidescr_t* dtmp;
    
    iniassert( ini );
    iniassert( ini->inifree );
    
    free = ini->inifree;
    
    s = ini->firstSect;
    // free sect
    while( s ) {
        stmp = s;
        s = s->next;
        IniFreeSect(stmp);
    }
    
    d = ini->filenames;
    // free filenames
    while( d ) {
        s = d->gsect;
        
        IniFreeSect(s);
        
        free( d->filename );
        
        dtmp = d;
        d = d->next;
        free(dtmp);
    }
    
    memset( ini, 0, sizeof(ini_t) );
}

/*
================
IniClearErrors
================
*/
void IniClearErrors( ini_t* ini ) {
    iniassert( ini );
    
    if( ini->errbuf ) {
        ini->errbuf[0] = 0;
    }
    ini->numOfErrors = 0;
    ini->flags &= ~0x1;
    ini->bufFill = 0;
}

/*
================
IniSetParseComments
================
*/
void IniSetParseComments( ini_t* ini, unsigned char flag ) {
    iniassert( ini );
    ini->flags = flag ? ini->flags | 0x2 : ini->flags & ~0x2;
}

/*
================
IniSetKeyAlign
================
*/
void IniSetKeyAlign( ini_t* ini, unsigned char align ) {
    iniassert( ini );
    ini->flags = (((unsigned)align) << 24) | (ini->flags & 0xffffff);
}

/*
================
IniSetEmptyLineAfterSection
================
*/
void IniSetEmptyLineAfterSection( ini_t* ini, unsigned char flag ) {
    iniassert( ini );
    ini->flags = flag ? ini->flags | 0x8000 : ini->flags & ~0x8000;
}

/*
================
IniSetSpaceBeforeEqual
================
*/
void IniSetSpaceBeforeEqual( ini_t* ini, unsigned char flag ) {
    iniassert( ini );
    ini->flags = flag ? ini->flags | 0x800 : ini->flags & ~0x800;
}

/*
================
IniSetSpaceAfterEqual
================
*/
void IniSetSpaceAfterEqual( ini_t* ini, unsigned char flag ) {
    iniassert( ini );
    ini->flags = flag ? ini->flags | 0x2000 : ini->flags & ~0x2000;
}

/*
================
IniSetPrintComments
================
*/
void IniSetPrintComments( ini_t* ini, unsigned char flag ) {
    iniassert( ini );
    ini->flags = flag ? ini->flags | 0x4000 : ini->flags & ~0x4000;
}

/*
================
IniSetPrintEmptyLines
================
*/
void IniSetPrintEmptyLines( ini_t* ini, unsigned char flag ) {
    iniassert( ini );
    ini->flags = flag ? ini->flags | 0x1000 : ini->flags & ~0x1000;
}

/*
================
IniSetPrintFilenameInTop
================
*/
void IniSetPrintFilenameInTop( ini_t* ini, unsigned char flag ) {
    iniassert( ini );
    ini->flags = flag ? ini->flags | 0x400 : ini->flags & ~0x400;
}

/*
================
IniSetPrintFilenameInBottom
================
*/
void IniSetPrintFilenameInBottom( ini_t* ini, unsigned char flag ) {
    iniassert( ini );
    ini->flags = flag ? ini->flags | 0x200 : ini->flags & ~0x200;
}

/*
================
IniAppendDescr
================
*/
inidescr_t* IniAppendDescr( ini_t* ini, const char* filename ) {
    inidescr_t* d;
    
    iniassert( ini );
    iniassert( filename );
    iniassert( filename[0] != 0 );
    
    d = IniDescrCreate( ini, filename, -1 );
    IniAppendDescr_s( ini, d );
    return d;
}

/*
================
IniAppendSect
================
*/
inisect_t* IniAppendSect( inidescr_t* descr, const char* key ) {
    inisect_t* s;
    
    iniassert( descr );
    iniassert( key );
    iniassert( key[0] != 0 );
    
    s = IniSectCreate( descr->ini, 
        IniStringCreate( descr->ini, key, -1 ),
        NULL
    );
    IniAppendSect_s( descr, s );
    return s;
}

/*
================
IniAppendInclude
================
*/
iniparam_t* IniAppendInclude( inidescr_t* descr, const char* filename ) {
    iniassert( descr );
    iniassert( descr->gsect );
    iniassert( descr->gsect->filename );
    iniassert( descr->gsect->filename->ini );
    iniassert( filename );
    iniassert( filename[0] != 0 );
    
    return IniAppendIncludeToSect( descr->gsect, filename );
}

/*
================
IniAppendIncludeToSect
================
*/
iniparam_t* IniAppendIncludeToSect( inisect_t* sect, const char* filename ) {
    ini_t* ini;
    iniparam_t* p;
    
    iniassert( sect );
    iniassert( sect->filename );
    iniassert( sect->filename->ini );
    iniassert( filename );
    iniassert( filename[0] != 0 );
    
    ini = sect->filename->ini;
    p = IniParamCreate(
        ini,
        IniStringCreate( ini, "#include", 8 ),
        IniStringCreate( ini, filename, -1 ),
        NULL
    );
    IniAppendParam_s( sect, p );
    return p;
}

/*
================
IniAppendParam
================
*/
iniparam_t* IniAppendParam( inisect_t* sect, const char* key, const char* val ) {
    ini_t* ini;
    iniparam_t* p;
    
    iniassert( sect );
    iniassert( sect->filename );
    iniassert( sect->filename->ini );
    iniassert( key );
    iniassert( key[0] != 0 );
    
    ini = sect->filename->ini;
    p = IniParamCreate(
        ini,
        IniStringCreate( ini, key, -1 ),
        IniStringCreate( ini, val, -1 ),
        NULL
    );
    IniAppendParam_s( sect, p );
    return p;
}

/*
================
IniAppendComment
================
*/
iniparam_t* IniAppendComment( inisect_t* sect, const char* comment ) {
    ini_t* ini;
    iniparam_t* p;
    
    iniassert( sect );
    iniassert( sect->filename );
    iniassert( sect->filename->ini );
    
    ini = sect->filename->ini;
    p = IniParamCreate(
        ini,
        NULL,
        NULL,
        IniStringCreate( ini, comment, -1 )
    );
    IniAppendParam_s( sect, p );
    return p;
}


/*
================
IniFindSect
================
*/
inisect_t* IniFindSect( ini_t* ini, const char* key ) {
    inisect_t* s;
    ptrdiff_t len;
    
    iniassert( ini );
    iniassert( key );
    iniassert( key[0] != 0 );
    
    s = ini->firstSect;
    len = (ptrdiff_t)strlen( key );
    while( s ) {
        if( len == s->key->length && !strncmp(key, s->key->string, len) ) {
            return s;
        }
        s = s->next;
    }
    return NULL;
}

/*
================
IniFindParam
================
*/
iniparam_t* IniFindParam( inisect_t* sect, const char* key ) {
    ptrdiff_t len;
    iniparam_t* p;
    
    iniassert( sect );
    iniassert( key );
    iniassert( key[0] != 0 );
    
    len = (ptrdiff_t)strlen( key );
    p = IniFindOnlyInSect( sect, key, len );
    if( !p ) {
        p = IniFindInInherit( sect, key, len );
    }
    return p;
}

/*
================
IniFind
================
*/
iniparam_t* IniFind( ini_t* ini, const char* sect, const char* param ) {
    inisect_t* found;
    
    iniassert( ini );
    iniassert( sect );
    iniassert( sect[0] != 0 );
    iniassert( param );
    iniassert( param[0] != 0 );
    
    found = IniFindSect( ini, sect );
    if( found ) {
        return IniFindParam( found, param );
    }
    return NULL;
}

/*
================
IniSectInherit
================
*/
int IniSectInherit( inisect_t* sect, const char* name ) {
    ini_t* ini;
    inisect_t* found;
    iniinh_t* inh;;
    
    iniassert( sect );
    iniassert( sect->filename );
    iniassert( sect->filename->ini );
    iniassert( name );
    iniassert( name[0] != 0 );
    
    ini = sect->filename->ini;
    found = IniFindSect( ini, name );
    
    if( !found || sect == found ) {
        return -1;
    }
    // Add to inherit
    if( sect->inherited ) {
        inh = sect->inherited;
        while( inh->next ) {
            inh = inh->next;
        }
        inh->next = IniInheritCreate( ini, found );
    } else {
        sect->inherited = IniInheritCreate( ini, found );
    }
    // Add to heirs
    if( found->heirs ) {
        inh = found->heirs;
        while( inh->next ) {
            inh = inh->next;
        }
        inh->next = IniHeirCreate( ini, sect );
    } else {
        found->heirs = IniHeirCreate( ini, sect );
    }
    return 0;
}

/*
================
IniLoad
================
*/
int IniLoad( ini_t* ini, const char* filename ) {
    iniassert( ini );
    iniassert( filename );
    iniassert( filename[0] != 0 );
    
    IniClearErrors( ini );
    return IniRecursiveParse( ini, filename, 0 );
}

/*
================
IniSaveToFile
================
*/
int IniSaveToFile( ini_t* ini, const char* filename ) {
    FILE* file;
    
    iniassert( ini );
    iniassert( filename );
    iniassert( filename[0] != 0 );
    
    IniClearErrors( ini );
    if( (file = fopen(filename, "w")) == NULL ) {
        IniPrint( ini, "error: can not open file for saving '%s'\n", filename );
        return -1;
    }
    
    IniFprint( file, ini, 1 );
    
    fclose(file);
    
    return 0;
}

/*
================
IniSave
================
*/
int IniSave( ini_t* ini ) {
    int ret = 0;
    inidescr_t* d;
    FILE* file;
    
    iniassert( ini );
    
    d = ini->filenames;
    
    IniClearErrors( ini );
    
    while( d ) {
        if( (file = fopen(d->filename->string, "w")) == NULL ) {
            IniPrint( ini, "error: can not open file for saving '%s'\n", d->filename->string );
            ret = -1;
        } else {
            IniFprintFiledescr( file, d, 0 );
            fclose(file);
        }
        d = d->next;
    }
    
    return ret;
}

/*
================
IniFirstFilename
================
*/
void* IniFirstFilename( inihandler_t* handler, ini_t* ini, fnIniFilter filter, void* userData ) {
    iniassert( handler );
    iniassert( ini );
    
    // Init
    handler->ini = ini;
    handler->inifilter = filter;
    handler->userData = userData;
    handler->descr = ini->filenames;
    handler->sect = NULL;
    handler->inh = NULL;
    handler->param = NULL;
    
    // If inifilter is defined then find first filename file name suitable 
    // for the filter
    if( handler->inifilter ) {
        while( handler->descr && !handler->inifilter( handler->descr, 
                handler->userData ) ) {
            handler->descr = handler->descr->next;
        }
    }
    
    // Check current filedescr
    if( handler->descr ) {
        handler->string = handler->descr->filename;
        handler->cstr = handler->string->string;
        handler->length = handler->string->length;
    } else {
        handler->string = NULL;
        handler->cstr = NULL;
        handler->length = 0;
        return NULL;
    }
        
    return handler;
}

/*
================
IniNextFilename
================
*/
void* IniNextFilename( void* h ) {
    inihandler_t* handler;
    
    iniassert( h );
    iniassert( ((inihandler_t*)h)->descr );
    
    handler = (inihandler_t*)h;
    handler->descr = handler->descr->next;
    
    // If inifilter is defined then find first filename file name suitable 
    // for the filter
    if( handler->inifilter ) {
        while( handler->descr && !handler->inifilter( handler->descr, 
                handler->userData ) ) {
            handler->descr = handler->descr->next;
        }
    }
    
    if( handler->descr ) {
        handler->string = handler->descr->filename;
        handler->cstr = handler->string->string;
        handler->length = handler->string->length;
    } else {
        handler->string = NULL;
        handler->cstr = NULL;
        handler->length = 0;
        return NULL;
    }
    return h;
}

/*
================
IniFirstSect
================
*/
void* IniFirstSect( inihandler_t* handler, ini_t* ini, fnIniFilter filter, void* userData ) {
    iniassert( handler );
    iniassert( ini );
    
    handler->ini = ini;
    handler->inifilter = filter;
    handler->userData = userData;
    handler->descr = NULL;
    handler->sect = ini->firstSect;
    handler->inh = NULL;
    handler->param = NULL;
    
    // If inifilter is defined then find first filename file name suitable 
    // for the filter
    if( handler->inifilter ) {
        while( handler->sect && !handler->inifilter( handler->sect, 
                handler->userData ) ) {
            handler->sect = handler->sect->next;
        }
    }
    
    // Check current filedescr
    if( handler->sect ) {
        handler->string = handler->sect->key;
        handler->cstr = handler->string->string;
        handler->length = handler->string->length;
    } else {
        handler->string = NULL;
        handler->cstr = NULL;
        handler->length = 0;
        return NULL;
    }
    return handler;
}

/*
================
IniNextSect
================
*/
void* IniNextSect( void* h ) {
    inihandler_t* handler;
    
    iniassert( h );
    iniassert( ((inihandler_t*)h)->sect );
    
    handler = (inihandler_t*)h;
    handler->sect = handler->sect->next;
    
    // If inifilter is defined then find first filename file name suitable 
    // for the filter
    if( handler->inifilter ) {
        while( handler->sect && !handler->inifilter( handler->sect, 
                handler->userData ) ) {
            handler->sect = handler->sect->next;
        }
    }
    
    if( handler->sect ) {
        handler->string = handler->sect->key;
        handler->cstr = handler->string->string;
        handler->length = handler->string->length;
    } else {
        handler->string = NULL;
        handler->cstr = NULL;
        handler->length = 0;
        return NULL;
    }
    return h;
}

/*
================
IniFirstSectInFile
================
*/
void* IniFirstSectInFile( inihandler_t* handler, inidescr_t* descr, fnIniFilter filter, void* userData ) {
    iniassert( handler );
    iniassert( descr );
    iniassert( descr->ini );
    
    handler->ini = descr->ini;
    handler->inifilter = filter;
    handler->userData = userData;
    handler->descr = NULL;
    handler->sect = descr->gsect;
    handler->inh = NULL;
    handler->param = NULL;
    
    // If inifilter is defined then find first filename file name suitable 
    // for the filter
    if( handler->inifilter ) {
        while( handler->sect && !handler->inifilter( handler->sect, 
                handler->userData ) ) {
            handler->sect = handler->sect->fnext;
        }
    }
    
    // Check current filedescr
    if( handler->sect ) {
        handler->string = handler->sect->key;
        handler->cstr = handler->string->string;
        handler->length = handler->string->length;
    } else {
        handler->string = NULL;
        handler->cstr = NULL;
        handler->length = 0;
        return NULL;
    }
    return handler;
}

/*
================
IniNextSectInFile
================
*/
void* IniNextSectInFile( void* h ) {
    inihandler_t* handler;
    
    iniassert( h );
    iniassert( ((inihandler_t*)h)->sect );
    
    handler = (inihandler_t*)h;
    handler->sect = handler->sect->fnext;
    
    // If inifilter is defined then find first filename file name suitable 
    // for the filter
    if( handler->inifilter ) {
        while( handler->sect && !handler->inifilter( handler->sect, 
                handler->userData ) ) {
            handler->sect = handler->sect->fnext;
        }
    }
    
    if( handler->sect ) {
        handler->string = handler->sect->key;
        handler->cstr = handler->string->string;
        handler->length = handler->string->length;
    } else {
        handler->string = NULL;
        handler->cstr = NULL;
        handler->length = 0;
        return NULL;
    }
    return h;
}

/*
================
IniFirstParam
================
*/
void* IniFirstParam( inihandler_t* handler, inisect_t* sect, fnIniFilter filter, void* userData ) {
    iniassert( handler );
    iniassert( sect );
    iniassert( sect->filename );
    iniassert( sect->filename->ini );
    
    handler->ini = sect->filename->ini;
    handler->inifilter = filter;
    handler->userData = userData;
    handler->descr = NULL;
    handler->inh = NULL;
    handler->param = sect->firstParam;
    
    // If inifilter is defined then find first filename file name suitable 
    // for the filter
    if( handler->inifilter ) {
        while( handler->param && (!handler->param->key || 
                !handler->inifilter( handler->param, 
                handler->userData )) ) {
            handler->param = handler->param->next;
        }
    }
    
    while( handler->param && !handler->param->key ) {
        handler->param = handler->param->next;
    }
    
    // Check current filedescr
    if( handler->param && handler->param->key ) {
        handler->string = handler->param->key;
        handler->cstr = handler->string->string;
        handler->length = handler->string->length;
    } else {
        handler->string = NULL;
        handler->cstr = NULL;
        handler->length = 0;
        return NULL;
    }
    return handler;
}

/*
================
IniNextParam
================
*/
void* IniNextParam( void* h ) {
    inihandler_t* handler;
    
    iniassert( h );
    iniassert( ((inihandler_t*)h)->param );
    
    handler = (inihandler_t*)h;
    handler->param = handler->param->next;
    
    // If inifilter is defined then find first filename file name suitable 
    // for the filter
    if( handler->inifilter ) {
        while( handler->param && (!handler->param->key || 
                !handler->inifilter( handler->param,
                handler->userData )) ) {
            handler->param = handler->param->next;
        }
    }
    
    while( handler->param && !handler->param->key ) {
        handler->param = handler->param->next;
    }
    
    if( handler->param && handler->param->key ) {
        handler->string = handler->param->key;
        handler->cstr = handler->string->string;
        handler->length = handler->string->length;
    } else {
        handler->string = NULL;
        handler->cstr = NULL;
        handler->length = 0;
        return NULL;
    }
    return h;
}

/*
================
IniFirstInherit
================
*/
void* IniFirstInherit( inihandler_t* handler, inisect_t* sect, fnIniFilter filter, void* userData ) {
    iniassert( handler );
    iniassert( sect );
    iniassert( sect->filename );
    iniassert( sect->filename->ini );
    
    handler->ini = sect->filename->ini;
    handler->inifilter = filter;
    handler->userData = userData;
    handler->descr = NULL;
    handler->inh = sect->inherited;
    handler->param = NULL;
    
    // If inifilter is defined then find first filename file name suitable 
    // for the filter
    if( handler->inifilter ) {
        while( handler->inh && !handler->inifilter( handler->inh, 
                handler->userData ) ) {
            handler->inh = handler->inh->next;
        }
    }
    
    // Check current filedescr
    if( handler->inh ) {
        handler->sect = handler->inh->sect;
        handler->string = handler->sect->key;
        handler->cstr = handler->string->string;
        handler->length = handler->string->length;
    } else {
        handler->sect = NULL;
        handler->string = NULL;
        handler->cstr = NULL;
        handler->length = 0;
        return NULL;
    }
    return handler;
}

/*
================
IniNextInherit
================
*/
void* IniNextInherit( void* h ) {
    inihandler_t* handler;
    
    iniassert( h );
    iniassert( ((inihandler_t*)h)->inh );
    
    handler = (inihandler_t*)h;
    handler->inh = handler->inh->next;
    
    // If inifilter is defined then find first filename file name suitable 
    // for the filter
    if( handler->inifilter ) {
        while( handler->inh && !handler->inifilter( handler->inh,
                handler->userData ) ) {
            handler->inh = handler->inh->next;
        }
    }
    
    if( handler->inh ) {
        handler->sect = handler->inh->sect;
        handler->string = handler->sect->key;
        handler->cstr = handler->string->string;
        handler->length = handler->string->length;
    } else {
        handler->sect = NULL;
        handler->string = NULL;
        handler->cstr = NULL;
        handler->length = 0;
        return NULL;
    }
    return h;
}

/*
================
IniFirstHeir
================
*/
void* IniFirstHeir( inihandler_t* handler, inisect_t* sect, fnIniFilter filter, void* userData ) {
    iniassert( handler );
    iniassert( sect );
    iniassert( sect->filename );
    iniassert( sect->filename->ini );
    
    handler->ini = sect->filename->ini;
    handler->inifilter = filter;
    handler->userData = userData;
    handler->descr = NULL;
    handler->inh = sect->heirs;
    handler->param = NULL;
    
    // If inifilter is defined then find first filename file name suitable 
    // for the filter
    if( handler->inifilter ) {
        while( handler->inh && !handler->inifilter( handler->inh, 
                handler->userData ) ) {
            handler->inh = handler->inh->next;
        }
    }
    
    // Check current filedescr
    if( handler->inh ) {
        handler->sect = handler->inh->sect;
        handler->string = handler->sect->key;
        handler->cstr = handler->string->string;
        handler->length = handler->string->length;
    } else {
        handler->sect = NULL;
        handler->string = NULL;
        handler->cstr = NULL;
        handler->length = 0;
        return NULL;
    }
    return handler;
}

/*
================
IniNextHeir
================
*/
void* IniNextHeir( void* h ) {
    /*inihandler_t* handler;
    
    iniassert( h );
    iniassert( ((inihandler_t*)h)->inh );
    
    handler = (inihandler_t*)h;
    handler->inh = handler->inh->next;
    
    // If inifilter is defined then find first filename file name suitable 
    // for the filter
    if( handler->inifilter ) {
        while( handler->inh && !handler->inifilter( handler->inh,
                handler->userData ) ) {
            handler->inh = handler->inh->next;
        }
    }
    
    if( handler->inh ) {
        handler->sect = handler->inh->sect;
        handler->string = handler->sect->key;
        handler->cstr = handler->string->string;
        handler->length = handler->string->length;
    } else {
        handler->sect = NULL;
        handler->string = NULL;
        handler->cstr = NULL;
        handler->length = 0;
        return NULL;
    }
    return h;*/

    return IniNextInherit( h );
}

/*
================
IniRead4fv
================
*/
int IniRead4fv( iniparam_t* param, float* fv ) {
    iniassert( param );
    iniassert( param->value );
    iniassert( fv );
    
    // Using (2 + 2) instead of the number 4, this is for suppression of
    // warning message of PVS-Studio (message of PVS-Studio:digit 4 is 
    // magic number)
    if( sscanf( param->value->string, "%f,%f,%f,%f", 
        fv, fv + 1, fv + 2, fv + 3 ) != (2 + 2) ) {
        return -1;
    }
    return 0;
}

/*
================
IniRead3fv
================
*/
int IniRead3fv( iniparam_t* param, float* fv ) {
    iniassert( param );
    iniassert( param->value );
    iniassert( fv );
    
    if( sscanf( param->value->string, "%f,%f,%f", fv, fv + 1, fv + 2 ) != 3 ) {
        return -1;
    }
    return 0;
}

/*
================
IniRead2fv
================
*/
int IniRead2fv( iniparam_t* param, float* fv ) {
    iniassert( param );
    iniassert( param->value );
    iniassert( fv );
    
    if( sscanf( param->value->string, "%f,%f", fv, fv + 1 ) != 2 ) {
        return -1;
    }
    return 0;
}

/*
================
IniRead1fv
================
*/
int IniRead1fv( iniparam_t* param, float* fv ) {
    iniassert( param );
    iniassert( param->value );
    iniassert( fv );
    
    if( sscanf( param->value->string, "%f", fv ) != 1 ) {
        return -1;
    }
    return 0;
}

/*
================
IniRead4iv
================
*/
int IniRead4iv( iniparam_t* param, int* iv ) {
    iniassert( param );
    iniassert( param->value );
    iniassert( iv );
    
    // Using (2 + 2) instead of the number 4, this is for suppression of
    // warning message of PVS-Studio (message of PVS-Studio:digit 4 is 
    // magic number)
    if( sscanf( param->value->string, "%d,%d,%d,%d", 
        iv, iv + 1, iv + 2, iv + 3 ) != (2 + 2) ) {
        return -1;
    }
    return 0;
}

/*
================
IniRead3iv
================
*/
int IniRead3iv( iniparam_t* param, int* iv ) {
    iniassert( param );
    iniassert( param->value );
    iniassert( iv );
    
    if( sscanf( param->value->string, "%d,%d,%d", iv, iv + 1, iv + 2 ) != 3 ) {
        return -1;
    }
    return 0;
}

/*
================
IniRead2iv
================
*/
int IniRead2iv( iniparam_t* param, int* iv ) {
    iniassert( param );
    iniassert( param->value );
    iniassert( iv );
    
    if( sscanf( param->value->string, "%d,%d", iv, iv + 1 ) != 2 ) {
        return -1;
    }
    return 0;
}

/*
================
IniRead1iv
================
*/
int IniRead1iv( iniparam_t* param, int* iv ) {
    iniassert( param );
    iniassert( param->value );
    iniassert( iv );
    
    if( sscanf( param->value->string, "%d", iv ) != 1 ) {
        return -1;
    }
    return 0;
}


/*
================
IniReadBool
================
*/
int IniReadBool( iniparam_t* param, unsigned char* b ) {
    iniassert( param );
    iniassert( param->value );
    iniassert( b );
    
    return IniScanBool( param->value->string, NULL, b );
}

/*
================
IniReadBoolv
================
*/
int IniReadBoolv( iniparam_t* param, unsigned char* b, ptrdiff_t n ) {
    ptrdiff_t i;
    char* s;
    
    iniassert( param );
    iniassert( param->value );
    iniassert( b );
    iniassert( n > 0 );
    
    s = param->value->string;
    for( i = 0; i < n; i++ ) {
        if( IniScanBool( s, &s, b + i ) ) {
            return -1;
        }
        IniSkipSpaces( &s );
        if( *s == ',' ) {
            s++;
        } else if( i < n-1 ) {
            return -1;
        }
    }
    return 0;
}

/*
================
IniReadString
================
*/
int IniReadString( iniparam_t* param, char* s ) {
    iniassert( param );
    iniassert( param->value );
    iniassert( s );
    
    return IniScanString( param->value->string, NULL, s );
}

/*
================
IniReadStringv
================
*/
int IniReadStringv( iniparam_t* param, char** sv, ptrdiff_t n ) {
    ptrdiff_t i;
    char* s;
    
    iniassert( param );
    iniassert( param->value );
    iniassert( sv );
    iniassert( n > 0 );
    
    s = param->value->string;
    for( i = 0; i < n; i++ ) {
        if( IniScanString( s, &s, sv[i] ) ) {
            return -1;
        }
        IniSkipSpaces( &s );
        if( *s == ',' ) {
            s++;
        } else if( i < n-1 ) {
            return -1;
        }
    }
    return 0;
}
