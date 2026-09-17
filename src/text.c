#include "wx_text.h"
#include <string.h>
#include <ctype.h>
static const char* race(unsigned id){
    static const char* names[]={"adventurer","human","orc","dwarf","night elf","undead","tauren","gnome","troll"};
    return id<9?names[id]:names[0];
}
static const char* class_name(unsigned id){
    static const char* names[]={"adventurer","warrior","paladin","hunter","rogue","priest","adventurer","shaman","mage","warlock","adventurer","druid"};
    return id<12?names[id]:names[0];
}
unsigned wx_text_wrap(const char* text,char (*lines)[49],unsigned capacity,const WxTextIdentity* identity){
    if(!text||!lines||!capacity)return 0;
    memset(lines,0,capacity*49);unsigned row=0,column=0;const char* substitution=0;size_t substitute_left=0;
    while((substitute_left||*text)&&row<capacity){
        char c;
        if(substitute_left){c=*substitution++;substitute_left--;}
        else {
            if(text[0]=='$'&&text[1]){
                int token=tolower((unsigned char)text[1]);
                if(token=='n'||token=='c'||token=='r'){
                    substitution=token=='n'?(identity&&identity->name?identity->name:"Adventurer"):
                        token=='c'?class_name(identity?identity->character_class:0):race(identity?identity->race:0);
                    substitute_left=strlen(substitution);text+=2;continue;
                }
                if(token=='b'){text+=2;c='\n';}
                else if(token=='g'){
                    const char* split=strchr(text+2,':');const char* end=strchr(text+2,';');
                    if(split&&end&&split<end){
                        substitution=identity&&identity->gender?split+1:text+2;
                        substitute_left=(size_t)((identity&&identity->gender?end:split)-substitution);
                        text=end+1;continue;
                    }c=*text++;
                }else c=*text++;
            }else c=*text++;
        }
        if(c=='\r')continue;if(c=='\t')c=' ';
        if(c=='\n'){row++;column=0;continue;}
        if(column==48){
            unsigned split=column;while(split&&lines[row][split-1]!=' ')split--;
            if(row+1==capacity)break;
            if(split&&c!=' '){
                unsigned carry=column-split;memcpy(lines[row+1],lines[row]+split,carry);memset(lines[row]+split-1,0,49-(split-1));
                row++;column=carry;
            }else {row++;column=0;}
        }
        if(c==' '&&!column)continue;
        lines[row][column++]=c;
    }
    return row<capacity?row+1:capacity;
}
