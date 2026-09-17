#ifndef WX_CHARACTER_H
#define WX_CHARACTER_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct WxCharacter {
    uint64_t guid;
    char name[49];
    uint8_t race,character_class,gender,level;
    uint8_t skin,face,hair_style,hair_color,facial_hair;
    uint32_t zone,map,flags,display[20];
    uint8_t inventory_type[20];
    float x,y,z;
} WxCharacter;
typedef struct WxCharacters {unsigned count;WxCharacter items[10];} WxCharacters;
int wx_characters_parse(const uint8_t* data,size_t size,WxCharacters* result);
typedef struct WxCharacterDraft {
    char name[13];
    uint8_t race,character_class,gender,skin,face,hair_style,hair_color,facial_hair;
} WxCharacterDraft;
int wx_character_class_allowed(unsigned race,unsigned character_class);
unsigned wx_character_create_encode(const WxCharacterDraft* draft,uint8_t* output,unsigned capacity);
const char* wx_race_name(unsigned race);
const char* wx_class_name(unsigned character_class);
const char* wx_character_create_result(unsigned code);
enum {WX_CHAR_DELETE_SUCCESS=0x39,WX_CHAR_DELETE_FAILED=0x3a,WX_CHAR_DELETE_TRANSFER=0x3b,
      WX_CHAR_DELETE_UNCONFIRMED=0x100};
const WxCharacter* wx_character_find(const WxCharacters* characters,uint64_t guid);
const char* wx_character_delete_result(unsigned code);
const char* wx_character_login_result(unsigned code);
#ifdef __cplusplus
}
#endif
#endif
