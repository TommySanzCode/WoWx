#ifndef WX_AUTH_H
#define WX_AUTH_H
#include <stddef.h>
#include <stdint.h>
#include "wx_realm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct WxAuthConfig {char host[64],username[32],password[32];uint32_t port;} WxAuthConfig;
typedef struct WxWorldSession {char host[256],username[32];uint8_t key[40];uint32_t port;} WxWorldSession;
enum {WX_AUTH_IDLE,WX_AUTH_CONNECTING,WX_AUTH_CHALLENGE,WX_AUTH_PROOF,WX_AUTH_REALMS,WX_AUTH_DONE,
    WX_AUTH_NETWORK_ERROR,WX_AUTH_PROTOCOL_ERROR,WX_AUTH_REJECTED,WX_AUTH_CRYPTO_ERROR,WX_AUTH_CANCELLED};
int wx_random_bytes(uint8_t* out,size_t count);
int wx_crypto_healthy(void);
int wx_auth_run(const WxAuthConfig* config);
unsigned wx_auth_state(void);
void wx_auth_cancel(void);
// Only the authentication worker may forget a session or start a new request.
void wx_auth_forget(void);
const char* wx_auth_status(void);
int wx_auth_session(WxWorldSession* session);
int wx_auth_realms(WxRealms* output);
int wx_auth_select(unsigned index,WxWorldSession* session);
int wx_world_probe(const WxWorldSession* session);
const char* wx_world_status(void);
void wx_network_start(void);
// Offline renderer fixture: network initialization only, no authentication worker.
void wx_network_telemetry_start(void);
int wx_network_ready(void);
#ifdef __cplusplus
}
#endif
#endif
