#include "wx_world.h"
#include "wx_game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Appearance check failed at %u\n",__LINE__);exit(1);}}while(0)
int main(void){
    static WxGame game;WxWorldView view={0},before;WxEntity entity={0};
    view.guid=1;view.equipment_display[16]=42;view.equipment_type[16]=14;
    entity.guid=1;entity.type=4;entity.fields[36]=0x010103;entity.player_bytes=0x04030201;entity.player_bytes2=0xAABBCC05;
    entity.fields[131]=50;entity.fields[132]=51;entity.fields[190]=0x400;entity.visible_items[16]=2362;
    wx_player_appearance(&view,&entity,&game);
    CHECK(view.race==3&&view.character_class==1&&view.gender==1&&view.skin==1&&view.face==2&&view.hair_style==3&&view.hair_color==4&&view.facial_hair==5);
    CHECK(view.appearance_revision==1&&view.equipment_entry[16]==2362&&!view.equipment_display[16]&&!view.equipment_type[16]);
    CHECK(view.equipment_ready_mask==(0x7ffff^(1u<<16))&&view.display_id==50&&view.native_display_id==51&&view.appearance_flags==0x400);
    entity.fields[22]=100;wx_player_appearance(&view,&entity,&game);CHECK(view.appearance_revision==1);
    game.item_names[0].id=2362;game.item_names[0].display=18730;game.item_names[0].inventory_type=14;
    wx_player_appearance(&view,&entity,&game);CHECK(!view.equipment_display[16]&&view.appearance_revision==1);
    game.item_names[0].metadata=1;wx_player_appearance(&view,&entity,&game);
    CHECK(view.equipment_display[16]==18730&&view.equipment_type[16]==14&&view.equipment_ready_mask==0x7ffff&&view.appearance_revision==2);
    before=view;entity.guid=2;wx_player_appearance(&view,&entity,&game);CHECK(!memcmp(&view,&before,sizeof view));
    entity.guid=1;entity.type=3;wx_player_appearance(&view,&entity,&game);CHECK(!memcmp(&view,&before,sizeof view));entity.type=4;
    entity.visible_items[16]=999;wx_player_appearance(&view,&entity,&game);CHECK(!view.equipment_display[16]&&view.appearance_revision==3);
    game.item_names[1].id=999;game.item_names[1].metadata=2;wx_player_appearance(&view,&entity,&game);
    CHECK(view.equipment_ready_mask==0x7ffff&&!view.equipment_display[16]&&view.appearance_revision==4);
    entity.visible_items[16]=0;entity.player_bytes=0;entity.player_bytes2=0;wx_player_appearance(&view,&entity,&game);
    CHECK(view.equipment_ready_mask==0x7ffff&&!view.equipment_entry[16]&&!view.skin&&!view.facial_hair&&view.appearance_revision==5);
    before=view;wx_player_appearance(0,&entity,&game);wx_player_appearance(&view,0,&game);wx_player_appearance(&view,&entity,0);
    CHECK(!memcmp(&view,&before,sizeof view));
    printf("Live player appearance checks: %u passed\n",checks);return 0;
}
