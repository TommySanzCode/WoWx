#ifndef WX_TEXT_H
#define WX_TEXT_H
#include <stddef.h>
typedef struct WxTextIdentity {const char* name;unsigned race,character_class,gender;} WxTextIdentity;
// Expands Vanilla character tokens, wraps at spaces, and always terminates rows.
// Returns the number of occupied rows, bounded by capacity.
unsigned wx_text_wrap(const char* text,char (*lines)[49],unsigned capacity,const WxTextIdentity* identity);
#endif
