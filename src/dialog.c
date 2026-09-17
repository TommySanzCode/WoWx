#include "wx_dialog.h"
#include "wx_text.h"
#include <pbkit/pbkit.h>
#include "wx_font.h"
#include <stdio.h>
#include <string.h>
static unsigned revision,selected,page;
static char lines[128][49];
static unsigned row_count(const WxDialog* d){
    if(d->screen==WX_SCREEN_QUESTS)return d->quest_count+d->gossip_count;
    if(d->screen==WX_SCREEN_REWARD)return d->choice_count;
    if(d->screen==WX_SCREEN_LOOT)return d->loot_count+(d->money?1:0);
    return 0;
}
void wx_dialog_input(const WxGame* game,const WxPad* pad,const WxEntity* player){
    const WxDialog* d=&game->dialog;if(!d->screen)return;
    if(revision!=d->revision){revision=d->revision;selected=page=0;}
    unsigned count=row_count(d);if(selected>=count)selected=count?count-1:0;
    if(pad->layer)return;
    if(pad->pressed&(1u<<WX_UP)){if(selected)selected--;}
    if(pad->pressed&(1u<<WX_DOWN)){if(selected+1<count)selected++;}
    if(pad->pressed&(1u<<WX_LEFT)){if(page)page--;}
    if(pad->pressed&(1u<<WX_RIGHT)){if(page<14)page++;}
    if(pad->pressed&(1u<<WX_B)){
        WxCommand close={d->screen==WX_SCREEN_LOOT?0x15f:0x190,d->guid,0,0};
        if(wx_world_command(&close))wx_world_close_dialog();return;
    }
    if(!(pad->pressed&(1u<<WX_A)))return;
    WxCommand command={0,d->guid,d->quest,0};
    switch(d->screen){
    case WX_SCREEN_QUESTS:
        if(!count)return;
        if(selected>=d->quest_count){
            const WxGossipOption* option=&d->gossip[selected-d->quest_count];if(option->coded)return;
            command.value=option->id;command.opcode=0x17c;break;
        }
        command.value=d->quests[selected].id;command.opcode=0x186;
        if(player)for(unsigned i=0;i<20;i++)if(player->quests[i*3]==command.value)command.opcode=0x18a;
        break;
    case WX_SCREEN_DETAILS:command.opcode=0x189;break;
    case WX_SCREEN_REQUEST:if(!d->can_complete)return;command.opcode=0x18c;break;
    case WX_SCREEN_REWARD:
        if(d->choice_count&&!strcmp(wx_item_name(game,d->choices[selected].item),"Loading item name..."))return;
        command.opcode=0x18e;command.extra=selected;break;
    case WX_SCREEN_LOOT:
        if(!count)return;
        if(d->money&&selected==0){command.opcode=0x15e;command.value=0;}
        else {const WxLootItem* item=&d->loot[selected-(d->money?1:0)];if(item->kind)return;command.opcode=0x108;command.value=item->slot;}
        break;
    default:return;
    }
    wx_world_command(&command);
}
void wx_dialog_draw(const WxGame* game,const WxWorldView* world){
    const WxDialog* d=&game->dialog;if(!d->screen)return;
    pb_fill(24,30,592,420,0xff17212a);wx_font_clear();
    wx_font_printat(1,3,"%.50s",d->title[0]?d->title:d->screen==WX_SCREEN_LOOT?"Loot":"Available quests");
    if(d->screen==WX_SCREEN_QUESTS){
        unsigned first=(selected/9)*9;
        for(unsigned i=first;i<row_count(d)&&i<first+9;i++)wx_font_printat(3+i-first,3,"%s %.46s",selected==i?">":" ",i<d->quest_count?d->quests[i].title:d->gossip[i-d->quest_count].text);
    }else if(d->screen==WX_SCREEN_LOOT){
        unsigned count=row_count(d),first=(selected/9)*9;
        for(unsigned i=first;i<count&&i<first+9;i++){
            if(d->money&&i==0)wx_font_printat(3+i-first,3,"%s %u copper",selected==i?">":" ",d->money);
            else {const WxLootItem* item=&d->loot[i-(d->money?1:0)];wx_font_printat(3+i-first,3,"%s %.38s x%u",selected==i?">":" ",wx_item_name(game,item->item),item->count);}
        }
        if(!count)wx_font_printat(4,3,"Nothing remains to loot.");
    }else if(d->screen==WX_SCREEN_REWARD){
        wx_font_printat(3,3,"%s",d->choice_count?"Choose a reward:":"Complete this quest:");
        for(unsigned i=0;i<d->choice_count;i++)wx_font_printat(4+i,3,"%s %.43s",selected==i?">":" ",wx_item_name(game,d->choices[i].item));
        wx_font_printat(11,3,"Reward: %u copper",d->money);
    }else{
        WxTextIdentity identity={world->name,world->race,world->character_class,world->gender};
        unsigned rows=wx_text_wrap(d->text,lines,128,&identity);
        if(d->objectives[0]&&rows<125){lines[rows++][0]=0;rows+=wx_text_wrap(d->objectives,lines+rows,128-rows,&identity);}
        unsigned pages=(rows+8)/9;if(page>=pages)page=pages?pages-1:0;
        for(unsigned i=0;i<9&&page*9+i<rows;i++)wx_font_printat(3+i,3,"%s",lines[page*9+i]);
        wx_font_printat(12,3,"Page %u/%u  D-pad left/right",page+1,pages);
    }
    const char* action=d->screen==WX_SCREEN_DETAILS?"Accept quest":d->screen==WX_SCREEN_REWARD?"Claim reward":d->screen==WX_SCREEN_REQUEST?(d->can_complete?"Continue":"Objectives incomplete"):d->screen==WX_SCREEN_LOOT?"Take selected":"Open quest";
    wx_font_printat(14,3,"A: %s  B: close",action);
    wx_font_printat(15,3,"%.51s",game->message);
}
