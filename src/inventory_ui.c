#include "wx_inventory_ui.h"
#include <pbkit/pbkit.h>
#include "wx_font.h"
#include <string.h>
static unsigned selected,equipment;
static unsigned filter(const WxInventory* inventory,unsigned* indices){
    unsigned count=0;
    for(unsigned i=0;i<inventory->count;i++){
        const WxInventoryItem* item=&inventory->items[i];unsigned gear=item->bag==255&&item->slot<23;
        if(gear==equipment)indices[count++]=i;
    }return count;
}
static unsigned vendor_revision,vendor_selected,vendor_selling,vendor_confirm;
static WxVendorItem pending_buy;static WxInventoryItem pending_sale;
static unsigned sale_items(const WxInventory* inventory,unsigned* indices){
    unsigned count=0;for(unsigned i=0;i<inventory->count;i++)
        if(inventory->items[i].bag!=255||inventory->items[i].slot>=23)indices[count++]=i;
    return count;
}
void wx_vendor_input(const WxInventory* inventory,const WxGame* game,const WxPad* pad){
    const WxDialog* d=&game->dialog;
    if(vendor_revision!=d->revision){vendor_revision=d->revision;vendor_selected=vendor_selling=vendor_confirm=0;}
    if(pad->layer)return;
    if(vendor_confirm){
        if(pad->pressed&(1u<<WX_B)){vendor_confirm=0;return;}
        if(pad->pressed&(1u<<WX_A)){
            WxCommand request={0,d->guid,0,0};
            if(vendor_selling){
                for(unsigned i=0;i<inventory->count;i++)if(inventory->items[i].guid==pending_sale.guid&&
                    (inventory->items[i].bag!=255||inventory->items[i].slot>=23)){
                    request.opcode=0x1a0;request.value=(uint32_t)pending_sale.guid;request.extra=(uint32_t)(pending_sale.guid>>32);break;}
            }else for(unsigned i=0;i<d->vendor_count;i++)if(d->vendor[i].slot==pending_buy.slot&&d->vendor[i].item==pending_buy.item&&
                d->vendor[i].price==pending_buy.price&&d->vendor[i].stock&&inventory->money>=pending_buy.price){
                request.opcode=0x1a2;request.value=pending_buy.item;request.extra=1;break;}
            if(request.opcode)wx_world_command(&request);vendor_confirm=0;
        }return;
    }
    if(pad->pressed&(1u<<WX_B)){wx_world_close_dialog();return;}
    if(pad->pressed&(1u<<WX_Y)){vendor_selling=!vendor_selling;vendor_selected=0;}
    unsigned indices[WX_INVENTORY_SLOTS],count=vendor_selling?sale_items(inventory,indices):d->vendor_count;
    if(vendor_selected>=count)vendor_selected=count?count-1:0;
    if(pad->pressed&(1u<<WX_UP)){if(vendor_selected)vendor_selected--;}
    if(pad->pressed&(1u<<WX_DOWN)){if(vendor_selected+1<count)vendor_selected++;}
    if(count&&(pad->pressed&(1u<<WX_A))){
        unsigned entry=vendor_selling?inventory->items[indices[vendor_selected]].entry:d->vendor[vendor_selected].item;
        if(!strcmp(wx_item_name(game,entry),"Loading item name..."))return;
        if(vendor_selling)pending_sale=inventory->items[indices[vendor_selected]];
        else {pending_buy=d->vendor[vendor_selected];if(!pending_buy.stock||inventory->money<pending_buy.price)return;}
        vendor_confirm=1;
    }
}
void wx_vendor_draw(const WxInventory* inventory,const WxGame* game){
    const WxDialog* d=&game->dialog;pb_fill(24,30,592,420,0xff17212a);wx_font_clear();
    wx_font_printat(1,3,"Merchant / %s / %u copper",vendor_selling?"sell":"buy",inventory->money);
    if(vendor_confirm){
        wx_font_printat(4,3,"%.48s",wx_item_name(game,vendor_selling?pending_sale.entry:pending_buy.item));
        if(vendor_selling)wx_font_printat(6,3,"Sell one item from this stack?");
        else wx_font_printat(6,3,"Buy %u for %u copper?",pending_buy.count,pending_buy.price);
        wx_font_printat(14,3,"A: confirm  B: cancel");return;
    }
    unsigned indices[WX_INVENTORY_SLOTS],count=vendor_selling?sale_items(inventory,indices):d->vendor_count,first=vendor_selected/8*8;
    for(unsigned i=first;i<count&&i<first+8;i++){
        if(vendor_selling){const WxInventoryItem* item=&inventory->items[indices[i]];
            wx_font_printat(3+i-first,3,"%s %.38s x%u",i==vendor_selected?">":" ",wx_item_name(game,item->entry),item->count);}
        else {const WxVendorItem* item=&d->vendor[i];wx_font_printat(3+i-first,3,"%s %.29s x%u %uc",i==vendor_selected?">":" ",wx_item_name(game,item->item),item->count,item->price);}
    }
    if(!count)wx_font_printat(4,3,"No items in this view.");
    if(!vendor_selling&&count&&vendor_selected<count){const WxVendorItem* item=&d->vendor[vendor_selected];
        if(!item->stock)wx_font_printat(12,3,"Out of stock");else if(item->price>inventory->money)wx_font_printat(12,3,"Not enough money");
        else if(item->stock!=UINT32_MAX)wx_font_printat(12,3,"Stock: %u",item->stock);}
    wx_font_printat(14,3,"A: %s  Y: %s  B: close",vendor_selling?"sell one":"buy",vendor_selling?"buy":"sell");
    wx_font_printat(15,3,"%.51s",game->message);
}
void wx_inventory_input(const WxInventory* inventory,const WxPad* pad){
    if(pad->layer)return;
    if(pad->pressed&(1u<<WX_Y)){equipment=!equipment;selected=0;}
    unsigned indices[WX_INVENTORY_SLOTS],count=filter(inventory,indices);
    if(selected>=count)selected=count?count-1:0;
    if(pad->pressed&(1u<<WX_UP)){if(selected)selected--;}
    if(pad->pressed&(1u<<WX_DOWN)){if(selected+1<count)selected++;}
    if(count&&!equipment&&(pad->pressed&(1u<<WX_A))){
        const WxInventoryItem* item=&inventory->items[indices[selected]];
        WxCommand equip={0x10a,0,item->bag,item->slot};wx_world_command(&equip);
    }
    if(count&&equipment&&(pad->pressed&(1u<<WX_X))){
        const WxInventoryItem* item=&inventory->items[indices[selected]];
        WxCommand store={0x10b,0,item->bag,item->slot};wx_world_command(&store);
    }
    if(count&&!equipment&&(pad->pressed&(1u<<WX_X))){
        const WxInventoryItem* item=&inventory->items[indices[selected]];
        WxCommand use={0xab,0,item->bag,item->slot};wx_world_command(&use);
    }
}
void wx_inventory_draw(const WxInventory* inventory,const WxGame* game){
    pb_fill(24,30,592,420,0xff17212a);wx_font_clear();
    wx_font_printat(1,3,"%s / %u gold %u silver %u copper",equipment?"Equipment":"Bags",inventory->money/10000,inventory->money/100%100,inventory->money%100);
    unsigned indices[WX_INVENTORY_SLOTS],count=filter(inventory,indices),first=selected/9*9;
    for(unsigned i=first;i<count&&i<first+9;i++){
        const WxInventoryItem* item=&inventory->items[indices[i]];
        wx_font_printat(3+i-first,3,"%s %.39s x%u",selected==i?">":" ",wx_item_name(game,item->entry),item->count);
    }
    if(!count)wx_font_printat(4,3,"No items in this view.");
    if(count&&selected<count){
        const WxInventoryItem* item=&inventory->items[indices[selected]];
        if(item->maximum_durability)wx_font_printat(12,3,"Durability %u/%u",item->durability,item->maximum_durability);
    }
    wx_font_printat(14,3,"%s  Y: %s  B: close",equipment?"X: put in bags":"A: equip X: use",equipment?"bags":"equipment");
    wx_font_printat(15,3,"%.51s",game->message);
}
