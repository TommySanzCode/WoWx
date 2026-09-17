#ifndef WX_REALM_H
#define WX_REALM_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Vanilla has a one-byte count and a two-byte packet length.
#define WX_REALMS_MAX 255u
#define WX_REALMS_BYTES 65535u
typedef struct WxRealm {
    char name[256],host[256];
    uint32_t type;
    float population;
    uint16_t port;
    uint8_t flags,characters,category,id;
} WxRealm;
typedef struct WxRealms {unsigned count;WxRealm items[WX_REALMS_MAX];} WxRealms;
// Parses the payload after command/length. Failure leaves output untouched.
int wx_realms_parse(const uint8_t* data,size_t size,WxRealms* output);
int wx_realm_available(const WxRealm* realm);
const char* wx_realm_type(uint32_t type);
const char* wx_realm_population(const WxRealm* realm);
#ifdef __cplusplus
}
#endif
#endif
