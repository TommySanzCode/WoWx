// Fixed-capacity Vanilla character enumeration adapter. Layout follows WoWee's
// parseCharEnumPreWotlk without the later expansions' equipment enchantment field.
#include "wx_character.h"
#include <string.h>
#include <math.h>
static uint32_t word(const uint8_t* p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float real(const uint8_t* p){uint32_t bits=word(p);float n;memcpy(&n,&bits,4);return n;}
int wx_character_class_allowed(unsigned race,unsigned c){
    // Vanilla combinations; masks address protocol class IDs, not UI indices.
    static const unsigned masks[]={0,0x336,0x29a,0x3e,0x83a,0x332,0x88a,0x312,0x1ba};
    return race>=1&&race<=8&&c<12&&(masks[race]&(1u<<c))!=0;
}
const char* wx_race_name(unsigned race){static const char* names[]={"Unknown","Human","Orc","Dwarf","Night Elf","Undead","Tauren","Gnome","Troll"};return names[race<=8?race:0];}
const char* wx_class_name(unsigned c){static const char* names[]={"Unknown","Warrior","Paladin","Hunter","Rogue","Priest","Unknown","Shaman","Mage","Warlock","Unknown","Druid"};return names[c<=11?c:0];}
unsigned wx_character_create_encode(const WxCharacterDraft* d,uint8_t* out,unsigned capacity){
    if(!d||!out||!wx_character_class_allowed(d->race,d->character_class)||d->gender>1)return 0;
    unsigned n=0;while(n<sizeof d->name&&d->name[n]){
        char c=d->name[n];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')))return 0;n++;
    }
    if(n<2||n>12||capacity<n+10)return 0;
    memcpy(out,d->name,n+1);uint8_t values[]={d->race,d->character_class,d->gender,d->skin,d->face,d->hair_style,d->hair_color,d->facial_hair,0};
    memcpy(out+n+1,values,sizeof values);return n+10;
}
const char* wx_character_create_result(unsigned code){
    switch(code){case 0:return "";case 0x2e:return "Character created";case 0x31:return "That name is already in use";
    case 0x32:return "Character creation is disabled";case 0x33:return "Realm faction restriction";
    case 0x34:return "This realm's character limit was reached";case 0x35:return "This account's character limit was reached";
    case 0x45:return "Enter a character name";case 0x46:return "That name is too short";case 0x47:return "That name is too long";
    case 0x48:return "That name contains an invalid character";case 0x4a:case 0x4b:return "That name is not allowed";
    default:return "The server rejected this name or character";}
}
const WxCharacter* wx_character_find(const WxCharacters* characters,uint64_t guid){
    if(!characters||!guid||characters->count>10)return NULL;
    for(unsigned i=0;i<characters->count;i++)if(characters->items[i].guid==guid)return characters->items+i;
    return NULL;
}
const char* wx_character_delete_result(unsigned code){
    switch(code){
    case WX_CHAR_DELETE_SUCCESS:return "Character deleted";
    case WX_CHAR_DELETE_FAILED:return "Server refused deletion; check guild leadership";
    case WX_CHAR_DELETE_TRANSFER:return "Character is locked for transfer";
    default:return "Deletion not confirmed; reconnect to check the character list";
    }
}
const char* wx_character_login_result(unsigned code){
    switch(code){
    case 0x3e:return "World server unavailable; choose a character to retry";
    case 0x3f:return "Character is already logging in";
    case 0x40:return "No instance server available; try again later";
    case 0x41:return "Unable to enter the world; try again";
    case 0x42:return "Character login is disabled";
    case 0x43:return "Character not found; the list has been refreshed";
    case 0x44:return "Character is locked for transfer";
    default:return "Character login failed";
    }
}
int wx_characters_parse(const uint8_t* data,size_t size,WxCharacters* out){
    if(!out)return 0;memset(out,0,sizeof *out);
    if(!data||!size||data[0]>10)return 0;
    unsigned count=data[0];size_t at=1;
    for(unsigned i=0;i<count;i++){
        if(at>size||size-at<8)return 0;WxCharacter* c=&out->items[i];
        c->guid=word(data+at)|((uint64_t)word(data+at+4)<<32);at+=8;if(!c->guid)return 0;
        for(unsigned j=0;j<i;j++)if(out->items[j].guid==c->guid)return 0;
        size_t start=at;while(at<size&&data[at]&&at-start<sizeof c->name)at++;
        if(at==size||at==start||at-start>=sizeof c->name)return 0;
        memcpy(c->name,data+start,at-start);at++;
        if(size-at<150)return 0;
        c->race=data[at];c->character_class=data[at+1];c->gender=data[at+2];c->level=data[at+8];
        c->skin=data[at+3];c->face=data[at+4];c->hair_style=data[at+5];c->hair_color=data[at+6];c->facial_hair=data[at+7];
        if(!c->race||c->race>8||!c->character_class||c->character_class>11||c->character_class==6||c->character_class==10||c->gender>1||!c->level||c->level>60)return 0;
        c->zone=word(data+at+9);c->map=word(data+at+13);
        c->x=real(data+at+17);c->y=real(data+at+21);c->z=real(data+at+25);c->flags=word(data+at+33);
        if(!isfinite(c->x)||!isfinite(c->y)||!isfinite(c->z)||fabsf(c->x)>20000||fabsf(c->y)>20000||fabsf(c->z)>20000)return 0;
        for(unsigned slot=0;slot<20;slot++){c->display[slot]=word(data+at+50+slot*5);c->inventory_type[slot]=data[at+54+slot*5];}
        at+=150;
    }
    if(at!=size)return 0;out->count=count;return 1;
}
