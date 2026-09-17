#ifndef WX_LOBBY_H
#define WX_LOBBY_H
#include "wx_character.h"
#include "wx_runtime.h"
#include "wx_ui.h"
#include "wx_looks.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {WX_LOBBY_CLOSED,WX_LOBBY_LOADING,WX_LOBBY_READY,WX_LOBBY_PENDING,WX_LOBBY_FAILED};
enum {WX_LOBBY_SELECT=1,WX_LOBBY_CREATE,WX_LOBBY_REFRESH,WX_LOBBY_CANCEL,WX_LOBBY_DELETE};
typedef struct WxCharacterLobby {
    WxCharacters characters;
    unsigned phase,revision,result_revision,result;
    uint64_t preferred;
    unsigned result_kind,pending_kind;
} WxCharacterLobby;
typedef struct WxLobbyCommand {unsigned kind;uint64_t guid;WxCharacterDraft draft;} WxLobbyCommand;
void wx_world_characters_open(void);
void wx_world_characters(WxCharacterLobby* output);
int wx_world_character_command(const WxLobbyCommand* command);
enum {WX_CHARACTER_LIST,WX_CHARACTER_CREATE,WX_CHARACTER_KEYBOARD,WX_CHARACTER_CONFIRM,
      WX_CHARACTER_DELETE,WX_CHARACTER_DELETE_KEYBOARD,WX_CHARACTER_APPEARANCE};
typedef struct WxCharacterUi {
    WxCharacterDraft draft;
    char original_name[13],message[80];
    unsigned screen,selected,row,key,open,result_revision;
    uint64_t delete_guid;
    char delete_name[49],delete_text[7];
    const WxLooks* looks;unsigned appearance_row;
    uint32_t random_state;unsigned randomizations;
} WxCharacterUi;
unsigned wx_character_appearance_choices(const WxCharacterUi* ui,unsigned field,unsigned* selected);
int wx_character_delete_ready(const WxCharacterUi* ui,const WxCharacterLobby* lobby);
void wx_character_ui_sync(WxCharacterUi* ui,const WxCharacterLobby* lobby);
int wx_character_ui_input(WxCharacterUi* ui,const WxCharacterLobby* lobby,const WxPad* pad,WxLobbyCommand* command);
void wx_character_ui_draw(const WxCharacterUi* ui,const WxCharacterLobby* lobby,WxUi* canvas);
#ifdef __cplusplus
}
#endif
#endif
