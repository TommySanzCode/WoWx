// Bounded local world session and repeatable login/logout integration scenario.
// Wire layouts follow pinned WoWee world_packets.cpp; cipher and Packet are reused.
#include "wx_auth.h"
#include "wx_world.h"
#include "wx_character.h"
#include "wx_game.h"
#include "wx_inventory.h"
#include "wx_cooldown.h"
#include "wx_cast.h"
#include "wx_actions.h"
#include "wx_lobby.h"
#include "wx_journal.h"
#include "net_connection.hpp"
#include "network/packet.hpp"
#include "auth/crypto.hpp"
#include "auth/vanilla_crypt.hpp"
#include "wx_srp.hpp"
#include <atomic>
#include <cstring>
#include <cmath>
using wowee::network::Packet;
enum Stage{OFFLINE,CONNECT_WORLD,AUTH_WORLD,CHARACTERS,CREATE_CHARACTER,LOGIN_WORLD,LOGOUT_WORLD,VERIFIED,FAILED,IN_WORLD,WORLD_TRANSFER,DELETE_CHARACTER};
static std::atomic<int> stage{OFFLINE};
static std::atomic_flag mailbox_lock=ATOMIC_FLAG_INIT;
static std::atomic<int> logout_requested{0},queue_failed{0};
static std::atomic<int> picker_requested{0};
static WxCharacterLobby lobby{};
static WxLobbyCommand lobby_command{};
static bool lobby_pending=false;
static uint64_t preferred_character=0;
static WxWorldView view{};
static WxWorldFault fault{};
static WxMovement movements[32];
static unsigned queue_read=0,queue_count=0;
static WxEntities entities{},entity_scratch{};
static uint8_t update_buffer[WX_UPDATE_LIMIT];
static WxEntity visible_entities[128];
static unsigned visible_count=0;
static float visible_origin[3];static uint64_t selected_guid=0;
static WxGame game{};
static WxCooldowns cooldowns{};
static WxCastState cast_state{};
static WxWorldClock world_clock{};
static WxWeather world_weather{};
static const WxCooldownCatalog* cooldown_catalog=nullptr;
static WxQuestCache quest_cache{};
static WxCommand commands[16];
static unsigned command_read=0,command_count=0;
static uint32_t requested_names[WX_ITEM_NAMES],requested_cursor;
static WxInventory inventory{};static bool inventory_dirty=true;
static uint64_t pending_sale=0;static unsigned sale_count=0,sale_money=0,sale_started=0;
struct Locked {
    Locked(){while(mailbox_lock.test_and_set(std::memory_order_acquire))Sleep(0);}
    ~Locked(){mailbox_lock.clear(std::memory_order_release);}
};
extern "C" void wx_world_clock(WxWorldClock* output){if(output){Locked lock;*output=world_clock;}}
extern "C" void wx_world_weather(WxWeather* output){if(output){Locked lock;*output=world_weather;}}
extern "C" void wx_world_view(WxWorldView* output){if(output){Locked lock;*output=view;}}
extern "C" void wx_world_fault(WxWorldFault* output){if(output){Locked lock;*output=fault;}}
extern "C" void wx_world_cooldown_catalog(const WxCooldownCatalog* catalog){Locked lock;cooldown_catalog=catalog;}
extern "C" void wx_world_cooldowns(const uint32_t bindings[8],WxCooldownView output[8],unsigned metrics[12]){
    Locked lock;
    uint32_t now=GetTickCount(); // sample after acquiring the lock, never before a newer packet timestamp
    if(bindings&&output)for(unsigned i=0;i<8;i++)output[i]=wx_cooldown_query(&cooldowns,cooldown_catalog,bindings[i],now);
    if(metrics){unsigned values[]={cooldowns.packets,cooldowns.revision,cooldowns.missing,cooldowns.overflow,sizeof cooldowns,cooldown_catalog?cooldown_catalog->version:0,
        cooldowns.modifier_updates,cooldowns.modifier_ambiguous,cooldowns.gcd_starts,cooldowns.gcd_cancels,(unsigned)(cooldowns.cast_speed*1000),cooldowns.player_family};memcpy(metrics,values,sizeof values);}
}
extern "C" void wx_world_game(WxGame* output){if(output){Locked lock;*output=game;}}
extern "C" void wx_world_cast(WxCastView* output){if(output){Locked lock;*output=wx_cast_query(&cast_state,GetTickCount());}}
extern "C" void wx_world_action_modifiers(const uint32_t bindings[8],WxActionModifiers output[8]){
    if(!bindings||!output)return;Locked lock;for(unsigned i=0;i<8;i++)wx_action_modifiers(&cooldowns,cooldown_catalog,bindings[i],&output[i]);
}
extern "C" int wx_world_quest(uint32_t id,WxQuestInfo* output){
    if(!output)return 0;Locked lock;const WxQuestInfo* q=wx_quest_cached(&quest_cache,id);
    if(!q)return 0;*output=*q;return 1;
}
extern "C" void wx_world_inventory(WxInventory* output){if(output){Locked lock;*output=inventory;}}
extern "C" void wx_world_characters(WxCharacterLobby* output){if(output){Locked lock;*output=lobby;}}
extern "C" void wx_world_characters_open(void){picker_requested=1;logout_requested=1;}
extern "C" int wx_world_character_command(const WxLobbyCommand* command){
    if(!command||command->kind<WX_LOBBY_SELECT||command->kind>WX_LOBBY_DELETE)return 0;
    uint8_t wire[22];if(command->kind==WX_LOBBY_CREATE&&!wx_character_create_encode(&command->draft,wire,sizeof wire))return 0;
    Locked lock;
    if(command->kind==WX_LOBBY_CANCEL&&lobby.phase==WX_LOBBY_FAILED){lobby.phase=WX_LOBBY_CLOSED;lobby.revision++;return 1;}
    if(lobby.phase!=WX_LOBBY_READY||lobby_pending)return 0;
    if(command->kind==WX_LOBBY_CREATE&&lobby.characters.count==10)return 0;
    if((command->kind==WX_LOBBY_SELECT||command->kind==WX_LOBBY_DELETE)&&!wx_character_find(&lobby.characters,command->guid))return 0;
    lobby_command=*command;lobby_pending=true;lobby.pending_kind=command->kind;lobby.phase=WX_LOBBY_PENDING;lobby.revision++;return 1;
}
static bool takeLobbyCommand(WxLobbyCommand& command){Locked lock;if(!lobby_pending)return false;command=lobby_command;lobby_pending=false;return true;}
extern "C" void wx_world_close_dialog(void){Locked lock;unsigned revision=game.dialog.revision+1;memset(&game.dialog,0,sizeof game.dialog);game.dialog.revision=revision;}
extern "C" int wx_world_command(const WxCommand* command){
    uint8_t buffer[32];if(wx_command_encode(command,buffer,sizeof buffer)<0)return 0;
    Locked lock;if(!view.active||command_count==16)return 0;
    if(command->opcode==0x128&&(!game.actions_ready||(command->value&&!wx_has_spell(&game,command->value))))return 0;
    if(command->opcode==0x13d)selected_guid=command->guid;
    commands[(command_read+command_count++)%16]=*command;return 1;
}
static bool takeCommand(WxCommand& command){Locked lock;if(!command_count)return false;
    command=commands[command_read];command_read=(command_read+1)%16;command_count--;return true;}
static uint32_t nextNameRequest(){
    Locked lock;const WxDialog& d=game.dialog;uint32_t ids[19+26+WX_INVENTORY_SLOTS+WX_VENDOR_ITEMS];unsigned count=0;
    for(unsigned i=0;i<19;i++)ids[count++]=view.equipment_entry[i];
    for(unsigned i=0;i<d.loot_count;i++)ids[count++]=d.loot[i].item;
    for(unsigned i=0;i<d.choice_count;i++)ids[count++]=d.choices[i].item;
    for(unsigned i=0;i<d.reward_count;i++)ids[count++]=d.rewards[i].item;
    for(unsigned i=0;i<inventory.count;i++)ids[count++]=inventory.items[i].entry;
    for(unsigned i=0;i<d.vendor_count;i++)ids[count++]=d.vendor[i].item;
    // Limit the working set to the cache size, in priority order. A full bag
    // plus large vendor must not cause endless eviction/query churn.
    unsigned unique=0;for(unsigned i=0;i<count&&unique<WX_ITEM_NAMES;i++){
        if(!ids[i])continue;unsigned j=0;while(j<unique&&ids[j]!=ids[i])j++;
        if(j==unique)ids[unique++]=ids[i];
    }
    for(unsigned i=0;i<unique;i++){
        unsigned j=0;while(j<WX_ITEM_NAMES&&!(game.item_names[j].id==ids[i]&&game.item_names[j].metadata)&&requested_names[j]!=ids[i])j++;
        if(ids[i]&&j==WX_ITEM_NAMES){requested_names[requested_cursor++%WX_ITEM_NAMES]=ids[i];return ids[i];}
    }return 0;
}
extern "C" unsigned wx_world_entity_count(void){Locked lock;return visible_count;}
extern "C" unsigned wx_world_entities(WxEntity* output,unsigned capacity){
    if(!output)return 0;Locked lock;unsigned count=visible_count<capacity?visible_count:capacity;
    memcpy(output,visible_entities,count*sizeof *output);return count;
}
static void publish_entities(uint32_t now){
    Locked lock;visible_count=0;
    if(inventory_dirty){wx_inventory_snapshot(&entities,view.guid,&inventory);inventory_dirty=false;}
    if(pending_sale&&wx_inventory_sale_completed(&inventory,pending_sale,sale_count,sale_money)){
        pending_sale=0;game.items_sold++;snprintf(game.message,sizeof game.message,"Sale complete");
    }else if(pending_sale&&now-sale_started>10000){pending_sale=0;snprintf(game.message,sizeof game.message,"Sale was not confirmed");}
    visible_count=wx_entities_nearby(&entities,view.guid,selected_guid,visible_origin,now,visible_entities,WX_VISIBLE_ENTITIES);
    for(unsigned i=0;i<visible_count;i++)if(visible_entities[i].guid==view.guid){wx_player_appearance(&view,&visible_entities[i],&game);break;}
}
extern "C" void wx_world_logout(void){logout_requested=1;}
extern "C" int wx_world_submit(const WxMovement* movement,uint32_t position_revision){
    if(!wx_movement_valid(movement))return 0;Locked lock;if(!view.active||view.position_revision!=position_revision)return 0;
    visible_origin[0]=movement->x;visible_origin[1]=movement->y;visible_origin[2]=movement->z;
    // Coalesce positions, but retain movement-mode edges (jump/land/stop).
    if(queue_count){unsigned last=(queue_read+queue_count-1)%32;
        if(movements[last].flags==movement->flags){movements[last]=*movement;return 1;}}
    if(queue_count==32){queue_failed=1;return 0;}
    movements[(queue_read+queue_count++)%32]=*movement;return 1;
}
static bool takeMovement(WxMovement& movement){Locked lock;if(!queue_count)return false;
    movement=movements[queue_read];queue_read=(queue_read+1)%32;queue_count--;return true;}
static void deactivate(){Locked lock;view.active=0;queue_count=0;visible_count=0;command_count=0;selected_guid=0;pending_sale=0;memset(&game,0,sizeof game);wx_cooldown_reset(&cooldowns);memset(&world_clock,0,sizeof world_clock);wx_weather_reset(&world_weather);memset(&cast_state,0,sizeof cast_state);memset(&quest_cache,0,sizeof quest_cache);memset(&inventory,0,sizeof inventory);inventory_dirty=true;}
extern "C" const char* wx_world_status(void){
    static const char* labels[]={"offline","connecting","authenticating","character list","creating character","entering world","logging out","logged out / progress saved","session failed","online / movement connected","loading world transfer","deleting character"};
    return labels[stage.load()];
}
struct WorldConnection:Connection {
    wowee::auth::VanillaCrypt cipher;bool encrypted=false;
    bool write(uint16_t opcode,const Packet& body){
        const auto& bytes=body.getData();if(bytes.size()>65529)return false;
        unsigned length=(unsigned)bytes.size()+4;
        uint8_t header[6]={(uint8_t)(length>>8),(uint8_t)length,(uint8_t)opcode,(uint8_t)(opcode>>8),0,0};
        if(encrypted)cipher.encrypt(header,sizeof header);
        return sendExact(header,sizeof header)&&sendExact(bytes.data(),bytes.size());
    }
    bool read(uint16_t& opcode,Packet& packet){
        uint8_t header[4];if(!receive(header,sizeof header))return false;
        if(encrypted)cipher.decrypt(header,sizeof header);
        unsigned length=(unsigned(header[0])<<8)|header[1];opcode=header[2]|(uint16_t(header[3])<<8);
        if(length<2)return false;
        std::vector<uint8_t> data(length-2);if(!receive(data.data(),data.size()))return false;
        packet=Packet(opcode,std::move(data));
        {Locked lock;if(wx_clock_apply(&world_clock,opcode,packet.getData().data(),packet.getSize(),GetTickCount())==0||
            wx_weather_apply(&world_weather,opcode,packet.getData().data(),packet.getSize())==0)return false;}
        return true;
    }
    bool expect(uint16_t wanted,Packet& packet){
        for(int i=0;i<256;i++){uint16_t opcode;if(!read(opcode,packet))return false;if(opcode==wanted)return true;}
        return false;
    }
    // Never consume another character operation's reply as an unrelated packet.
    // The login failure opcode is an alternative to LOGIN_VERIFY_WORLD.
    bool characterReply(uint16_t wanted,Packet& packet,uint16_t* received=nullptr){
        for(unsigned i=0;i<256;i++){uint16_t opcode;if(!read(opcode,packet))return false;
            if(opcode==wanted||(wanted==0x236&&opcode==0x41)){if(received)*received=opcode;return true;}
            if(opcode==0x3a||opcode==0x3b||opcode==0x3c||opcode==0x41||opcode==0x236)return false;
            if(opcode==0x1dd&&packet.getSize()!=4)return false;
        }return false;
    }
};
static int failure(){deactivate();stage=FAILED;{Locked lock;
    if(lobby.pending_kind==WX_LOBBY_DELETE){lobby.result_kind=WX_LOBBY_DELETE;lobby.result=WX_CHAR_DELETE_UNCONFIRMED;lobby.result_revision++;}
    if(lobby.phase!=WX_LOBBY_CLOSED){lobby.phase=WX_LOBBY_FAILED;lobby.revision++;}
    lobby.pending_kind=0;lobby_pending=false;}return 0;}
static int packetFailure(unsigned reason,uint16_t opcode,const Packet& packet){
    {Locked lock;fault.revision++;fault.reason=reason;fault.opcode=opcode;fault.size=(uint32_t)packet.getSize();
        fault.captured=fault.size<sizeof fault.data?fault.size:sizeof fault.data;
        memcpy(fault.data,packet.getData().data(),fault.captured);}
    return failure();
}
static bool characterList(WorldConnection& c,Packet& packet,WxCharacters& characters){
    Packet empty;c.started=GetTickCount();c.timeout_ms=15000;
    return c.write(0x37,empty)&&c.characterReply(0x3b,packet)&&wx_characters_parse(packet.getData().data(),packet.getSize(),&characters);
}
static int chooseCharacter(WorldConnection& c,bool live,WxCharacter& chosen){
    bool manual=picker_requested.exchange(0)!=0;Packet packet;WxCharacters characters{};stage=CHARACTERS;
    if(manual){Locked lock;lobby.phase=WX_LOBBY_LOADING;lobby.revision++;lobby.preferred=preferred_character;}
    if(!characterList(c,packet,characters))return -1;
    // Probe mode retains its disposable bootstrap for existing protocol fixtures.
    // Live empty accounts always use the controller creation screen.
    if(!live&&!characters.count){
        stage=CREATE_CHARACTER;WxCharacterDraft draft{};strcpy(draft.name,"Xboxer");draft.race=draft.character_class=1;
        uint8_t data[22];unsigned size=wx_character_create_encode(&draft,data,sizeof data);Packet create;create.writeBytes(data,size);
        if(!c.write(0x36,create)||!c.characterReply(0x3a,packet)||packet.getSize()!=1||packet.readUInt8()!=0x2e||!characterList(c,packet,characters))return -1;
    }
    if(!manual){
        if(preferred_character)for(unsigned i=0;i<characters.count;i++)if(characters.items[i].guid==preferred_character){chosen=characters.items[i];return 1;}
        for(unsigned i=0;i<characters.count;i++)if(!strcmp(characters.items[i].name,"Xboxer")){chosen=characters.items[i];return 1;}
        if(!live)return -1;
    }
    {Locked lock;lobby.characters=characters;lobby.phase=WX_LOBBY_READY;lobby.pending_kind=0;lobby.preferred=preferred_character;lobby.revision++;}
    unsigned ping_at=GetTickCount(),sequence=0;
    while(1){
        if(logout_requested.load()){Locked lock;lobby.phase=WX_LOBBY_CLOSED;lobby.pending_kind=0;lobby.revision++;lobby_pending=false;return 0;}
        WxLobbyCommand command{};
        if(takeLobbyCommand(command)){
            if(command.kind==WX_LOBBY_CANCEL){Locked lock;lobby.phase=WX_LOBBY_CLOSED;lobby.pending_kind=0;lobby.revision++;return 0;}
            if(command.kind==WX_LOBBY_SELECT){
                for(unsigned i=0;i<characters.count;i++)if(characters.items[i].guid==command.guid){chosen=characters.items[i];return 1;}return -1;
            }
            unsigned result=0;
            if(command.kind==WX_LOBBY_CREATE){
                uint8_t data[22];unsigned size=wx_character_create_encode(&command.draft,data,sizeof data);if(!size)return -1;
                Packet create;create.writeBytes(data,size);stage=CREATE_CHARACTER;c.started=GetTickCount();c.timeout_ms=15000;
                if(!c.write(0x36,create)||!c.characterReply(0x3a,packet)||packet.getSize()!=1)return -1;
                result=packet.readUInt8();
                if(!((result>=0x2e&&result<=0x37)||(result>=0x45&&result<=0x51)))return -1;
                // A creation error leaves the roster and draft available for retry.
                if(result==0x2e&&!characterList(c,packet,characters))return -1;
            }else if(command.kind==WX_LOBBY_DELETE){
                if(!wx_character_find(&characters,command.guid))return -1;
                Packet remove;remove.writeUInt64(command.guid);stage=DELETE_CHARACTER;c.started=GetTickCount();c.timeout_ms=15000;
                if(!c.write(0x38,remove)||!c.characterReply(0x3c,packet)||packet.getSize()!=1)return -1;
                result=packet.readUInt8();if(result<WX_CHAR_DELETE_SUCCESS||result>WX_CHAR_DELETE_TRANSFER)return -1;
                // Refresh after every terminal response. A lost response/refresh is
                // uncertain: never retry a destructive request automatically.
                if(!characterList(c,packet,characters))return -1;
                if(result==WX_CHAR_DELETE_SUCCESS){
                    if(wx_character_find(&characters,command.guid))return -1;
                    Locked lock;if(preferred_character==command.guid)preferred_character=0;
                    if(lobby.preferred==command.guid)lobby.preferred=0;
                }
            }else if(command.kind==WX_LOBBY_REFRESH&&!characterList(c,packet,characters))return -1;
            {Locked lock;lobby.characters=characters;lobby.phase=WX_LOBBY_READY;lobby.pending_kind=0;lobby.revision++;
                if(command.kind==WX_LOBBY_CREATE||command.kind==WX_LOBBY_DELETE){lobby.result_kind=command.kind;lobby.result=result;lobby.result_revision++;}}
            stage=CHARACTERS;ping_at=GetTickCount();
        }
        unsigned now=GetTickCount();
        if(now-ping_at>=30000){Packet ping;ping.writeUInt32(++sequence);ping.writeUInt32(0);c.started=now;c.timeout_ms=5000;
            if(!c.write(0x1dc,ping))return -1;ping_at=now;}
        fd_set input;FD_ZERO(&input);FD_SET(c.handle,&input);timeval wait{0,0};
        int available=select((int)c.handle+1,&input,nullptr,nullptr,&wait);if(available<0)return -1;
        if(available){uint16_t opcode;c.started=now;c.timeout_ms=5000;
            if(!c.read(opcode,packet)||(opcode==0x1dd&&packet.getSize()!=4))return -1;
            // Character replies must correspond to the single in-flight request.
            if(opcode==0x3b||opcode==0x3a||opcode==0x3c||opcode==0x41||opcode==0x236)return -1;
        }
        Sleep(10);
    }
}
static int world_run(const WxWorldSession* session,bool live){
    deactivate();logout_requested=0;queue_failed=0;memset(&entities,0,sizeof entities);memset(requested_names,0,sizeof requested_names);requested_cursor=0;
    {Locked lock;lobby.phase=WX_LOBBY_CLOSED;lobby.pending_kind=lobby.result_kind=lobby.result=0;lobby_pending=false;}
    if(!session||!session->port||session->port>65535||!memchr(session->host,0,sizeof session->host)||!memchr(session->username,0,sizeof session->username))return failure();
    stage=CONNECT_WORLD;WorldConnection c;if(!c.connectTo(session->host,(uint16_t)session->port))return failure();
    Packet packet;
    if(!c.expect(0x1ec,packet)||packet.getSize()!=4)return failure();
    uint32_t serverSeed=packet.readUInt32(),clientSeed=0;
    if(!wx_random_bytes((uint8_t*)&clientSeed,4))return failure();
    Packet hash;hash.writeBytes((const uint8_t*)session->username,strlen(session->username));
    auto sessionKey=wx_srp_natural(session->key,40);if(sessionKey.empty())return failure();
    hash.writeUInt32(0);hash.writeUInt32(clientSeed);hash.writeUInt32(serverSeed);hash.writeBytes(sessionKey.data(),sessionKey.size());
    auto digest=wowee::auth::Crypto::sha1(hash.getData());if(digest.size()!=20)return failure();
    Packet auth;auth.writeUInt32(5875);auth.writeUInt32(1);auth.writeString(session->username);auth.writeUInt32(clientSeed);auth.writeBytes(digest.data(),20);auth.writeUInt32(0);
    stage=AUTH_WORLD;if(!c.write(0x1ed,auth))return failure();
    c.cipher.init(sessionKey);c.encrypted=true;
    if(!c.expect(0x1ee,packet)||!packet.hasRemaining(1)||packet.readUInt8()!=0x0c)return failure();
    Packet empty;WxCharacter chosen{};uint64_t guid=0;
    for(;;){
        int selection=chooseCharacter(c,live,chosen);
        if(selection<0)return failure();if(!selection){stage=VERIFIED;return 1;}guid=chosen.guid;
        stage=LOGIN_WORLD;Packet login;login.writeUInt64(guid);
        // Cold first-map loading has taken over 30 seconds on the local server.
        c.started=GetTickCount();c.timeout_ms=60000;uint16_t response=0;
        if(!c.write(0x3d,login)||!c.characterReply(0x236,packet,&response))return failure();
        if(response==0x236){if(packet.getSize()!=20)return failure();break;}
        if(!live||packet.getSize()!=1)return failure();
        unsigned result=packet.readUInt8();if(result<0x3e||result>0x44)return failure();
        {Locked lock;preferred_character=guid;lobby.preferred=guid;lobby.result_kind=WX_LOBBY_SELECT;
            lobby.result=result;lobby.result_revision++;lobby.pending_kind=0;lobby.phase=WX_LOBBY_LOADING;lobby.revision++;}
        // Re-enumerate and wait for a fresh user selection, never auto-retry login.
        picker_requested=1;
    }
    c.started=GetTickCount();c.timeout_ms=15000;
    uint32_t map=packet.readUInt32();float x=packet.readFloat(),y=packet.readFloat(),z=packet.readFloat(),orientation=packet.readFloat();
    if(map>65535||!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||!std::isfinite(orientation)||fabsf(x)>20000||fabsf(y)>20000||fabsf(z)>20000)return failure();
    {Locked lock;preferred_character=guid;lobby.preferred=guid;lobby.phase=WX_LOBBY_CLOSED;lobby.pending_kind=0;lobby.revision++;}
    if(live){
        Packet mover;mover.writeUInt64(guid);if(!c.write(0x26a,mover))return failure();
        {Locked lock;view.guid=guid;view.map=map;view.x=x;view.y=y;view.z=z;view.orientation=orientation;
            visible_origin[0]=x;visible_origin[1]=y;visible_origin[2]=z;
            memcpy(view.name,chosen.name,sizeof view.name);view.race=chosen.race;view.character_class=chosen.character_class;view.gender=chosen.gender;
            view.skin=chosen.skin;view.face=chosen.face;view.hair_style=chosen.hair_style;view.hair_color=chosen.hair_color;view.facial_hair=chosen.facial_hair;
            memcpy(view.equipment_display,chosen.display,sizeof view.equipment_display);memcpy(view.equipment_type,chosen.inventory_type,sizeof view.equipment_type);
            memset(view.equipment_entry,0,sizeof view.equipment_entry);view.equipment_ready_mask=0;
            view.appearance_flags=view.display_id=view.native_display_id=0;view.appearance_revision++;
            wx_cooldown_unit(&cooldowns,chosen.character_class,1,0);
            view.revision++;view.position_revision++;view.received=view.sent=0;view.active=1;}
        stage=IN_WORLD;
        uint32_t ping_at=GetTickCount(),sequence=0,move_at=0,flags=0,publish_at=0,item_at=0;
        WxMovement current{};bool have_movement=false;uint32_t transfer_started=0;
        while(!logout_requested.load()){
            if(queue_failed)return failure();
            uint32_t now=GetTickCount();
            if(transfer_started&&now-transfer_started>15000)return failure();
            WxCommand command;
            if(takeCommand(command)){
                if(command.opcode==0x1a0&&pending_sale){Locked lock;snprintf(game.message,sizeof game.message,"Wait for the previous sale");continue;}
                if(command.opcode==0x1a0){
                    Locked lock;uint64_t item=((uint64_t)command.extra<<32)|command.value;
                    for(unsigned i=0;i<inventory.count;i++)if(inventory.items[i].guid==item){
                        pending_sale=item;sale_count=inventory.items[i].count;sale_money=inventory.money;sale_started=now;break;}
                }
                uint8_t bytes[32];int length=wx_command_encode(&command,bytes,sizeof bytes);if(length<0)return failure();
                Packet body;body.writeBytes(bytes,length);c.started=now;c.timeout_ms=5000;
                if(!c.write(command.opcode,body))return failure();
                // Vanilla sends no successful edit acknowledgement. Publish only
                // after transport succeeds; the next login supplies authoritative bindings.
                if(command.opcode==0x128){Locked lock;game.actions[command.extra]=command.value;game.action_revision++;}
            }
            if(now-item_at>=50){
                item_at=now;
                if(uint32_t item=nextNameRequest()){
                    Packet query;query.writeUInt32(item);query.writeUInt64(0);c.started=now;c.timeout_ms=5000;
                    if(!c.write(0x56,query))return failure();
                }
            }
            if(takeMovement(current))have_movement=true;
            if(have_movement&&(current.flags!=flags||now-move_at>=100)){
                uint8_t bytes[44];unsigned size=wx_movement_encode(&current,bytes,sizeof bytes);if(!size)return failure();
                Packet movement;movement.writeBytes(bytes,size);c.started=now;c.timeout_ms=5000;
                if(!c.write(wx_movement_opcode(flags,current.flags),movement))return failure();
                flags=current.flags;move_at=now;have_movement=false;
                {Locked lock;view.sent++;}
            }
            if(now-ping_at>=30000){Packet ping;ping.writeUInt32(++sequence);ping.writeUInt32(0);c.started=now;c.timeout_ms=5000;
                if(!c.write(0x1dc,ping))return failure();ping_at=now;}
            // Bound incoming work per iteration so a busy peer cannot starve movement.
            for(unsigned i=0;i<16;i++){
                fd_set input;FD_ZERO(&input);FD_SET(c.handle,&input);timeval wait{0,0};
                int available=select((int)c.handle+1,&input,nullptr,nullptr,&wait);if(available<0)return failure();if(!available)break;
                c.started=GetTickCount();c.timeout_ms=5000;uint16_t opcode;
                if(!c.read(opcode,packet))return packetFailure(1,0,Packet{});
                {Locked lock;view.received++;}
                int handled;{Locked lock;handled=opcode==0x5d?wx_quest_cache_apply(&quest_cache,packet.getData().data(),packet.getSize()):wx_game_apply(&game,opcode,packet.getData().data(),packet.getSize(),guid);}
                if(handled==0)return packetFailure(2,opcode,packet);
                {Locked lock;handled=wx_cooldown_apply(&cooldowns,cooldown_catalog,&inventory,opcode,packet.getData().data(),packet.getSize(),guid,GetTickCount());}
                if(handled==0)return packetFailure(2,opcode,packet);
                {Locked lock;handled=wx_cast_apply(&cast_state,opcode,packet.getData().data(),packet.getSize(),guid,GetTickCount());}
                if(handled==0)return packetFailure(2,opcode,packet);
                if(opcode==0x58&&packet.getSize()>=4){uint32_t entry;memcpy(&entry,packet.getData().data(),4);entry&=0x7fffffff;
                    for(auto& requested:requested_names)if(requested==entry)requested=0;}
                if(opcode==0x1a1)pending_sale=0;
                if(opcode==0xa9||opcode==0x1f6){
                    const uint8_t* data=packet.getData().data();size_t size=packet.getSize();
                    if(opcode==0x1f6){if(!wx_update_inflate(data,size,update_buffer,sizeof update_buffer,&size))return packetFailure(3,opcode,packet);data=update_buffer;}
                    if(!wx_entities_apply(&entities,&entity_scratch,data,size,now))return packetFailure(4,opcode,packet);
                    inventory_dirty=true;
                    // Consume the new unit fields before the next packet in this
                    // read batch, not after the periodic visible-entity publish.
                    for(unsigned entity=0;entity<WX_MAX_ENTITIES;entity++)if(entities.items[entity].guid==guid){
                        const auto& self=entities.items[entity];float haste,ranged;
                        memcpy(&haste,&self.fields[145],4);memcpy(&ranged,&self.fields[128],4);
                        Locked lock;wx_cooldown_unit(&cooldowns,chosen.character_class,haste,ranged);break;
                    }
                }
                if(opcode==0xdd&&!wx_entities_monster_move(&entities,packet.getData().data(),packet.getSize(),now))return packetFailure(5,opcode,packet);
                if(opcode==0xaa){if(packet.getSize()!=8)return failure();wx_entities_destroy(&entities,packet.readUInt64());inventory_dirty=true;}
                if(opcode==0x4d){deactivate();stage=VERIFIED;return 1;}
                if(opcode==0x3f){
                    if((packet.getSize()!=4&&packet.getSize()!=12)||packet.readUInt32()>65535)return packetFailure(6,opcode,packet);
                    {Locked lock;view.active=0;queue_count=command_count=visible_count=0;game.attack_target=0;game.dialog.screen=0;wx_weather_reset(&world_weather);}
                    have_movement=false;transfer_started=now?now:1;stage=WORLD_TRANSFER;
                }
                if(opcode==0x3e){
                    if(packet.getSize()!=20)return packetFailure(6,opcode,packet);
                    uint32_t destinationMap=packet.readUInt32();float nx=packet.readFloat(),ny=packet.readFloat(),nz=packet.readFloat(),angle=packet.readFloat();
                    if(destinationMap>65535||!std::isfinite(nx)||!std::isfinite(ny)||!std::isfinite(nz)||!std::isfinite(angle)||
                        fabsf(nx)>20000||fabsf(ny)>20000||fabsf(nz)>20000)return packetFailure(6,opcode,packet);
                    // New map/instance creates replace the old entity set. The
                    // main thread consumes a new position revision before it can
                    // submit input, and terrain collision gates local walking.
                    memset(&entities,0,sizeof entities);inventory_dirty=true;
                    {Locked lock;if(stage!=WORLD_TRANSFER)wx_weather_reset(&world_weather);queue_count=command_count=visible_count=0;memset(&cast_state,0,sizeof cast_state);view.map=destinationMap;view.x=nx;view.y=ny;view.z=nz;
                        visible_origin[0]=nx;visible_origin[1]=ny;visible_origin[2]=nz;selected_guid=0;
                        view.orientation=angle;view.position_revision++;view.active=1;game.attack_target=0;game.dialog.screen=0;}
                    have_movement=false;flags=0;move_at=now;transfer_started=0;
                    if(!c.write(0xdc,empty))return packetFailure(6,opcode,packet);stage=IN_WORLD;
                }
                // Controller near teleport: packed GUID, counter, Vanilla
                // MovementInfo. The acknowledgement uses an unpacked GUID.
                if(opcode==0xc7){
                    const auto& bytes=packet.getData();if(bytes.empty()||bytes.size()>132)return packetFailure(6,opcode,packet);
                    unsigned packed=1;for(unsigned bit=0;bit<8;bit++)if(bytes[0]&(1u<<bit))packed++;
                    if(bytes.size()<packed+4)return packetFailure(6,opcode,packet);
                    uint32_t counter;memcpy(&counter,bytes.data()+packed,4);
                    uint8_t relocation[128];memcpy(relocation,bytes.data(),packed);
                    memcpy(relocation+packed,bytes.data()+packed+4,bytes.size()-packed-4);
                    WxEntity destination{};
                    if(!wx_relocation_decode(relocation,bytes.size()-4,&destination)||destination.guid!=guid)return packetFailure(6,opcode,packet);
                    uint32_t moveTime;memcpy(&moveTime,relocation+packed+4,4);
                    // Publish the authoritative destination and discard queued
                    // input from the prior position before acknowledging.
                    {Locked lock;queue_count=command_count=0;view.x=destination.x;view.y=destination.y;view.z=destination.z;
                        visible_origin[0]=destination.x;visible_origin[1]=destination.y;visible_origin[2]=destination.z;selected_guid=0;
                        view.orientation=destination.orientation;view.position_revision++;game.attack_target=0;}
                    have_movement=false;flags=0;move_at=now;
                    wx_entities_relocate(&entities,relocation,bytes.size()-4,nullptr);
                    Packet ack;ack.writeUInt64(guid);ack.writeUInt32(counter);ack.writeUInt32(moveTime);
                    if(!c.write(0xc7,ack))return packetFailure(6,opcode,packet);
                }
                if(opcode==0xc5){
                    uint64_t moved=0;if(!wx_entities_relocate(&entities,packet.getData().data(),packet.getSize(),&moved))return packetFailure(7,opcode,packet);
                    if(moved==guid)return packetFailure(6,opcode,packet);
                }
                if(opcode==0x1dd&&packet.getSize()!=4)return failure();
            }
            if(now-publish_at>=30){publish_entities(now);publish_at=now;}
            Sleep(10);
        }
    }
    deactivate();
    stage=LOGOUT_WORLD;c.started=GetTickCount();c.timeout_ms=40000;
    if(!c.write(0x4b,empty)||!c.expect(0x4d,packet))return failure();
    stage=VERIFIED;return 1;
}
extern "C" int wx_world_probe(const WxWorldSession* session){return world_run(session,false);}
extern "C" int wx_world_live(const WxWorldSession* session){return world_run(session,true);}
