#ifndef _stun_agent_h_
#define _stun_agent_h_

#include "sys/sock.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct stun_agent_t stun_agent_t;
typedef struct stun_request_t stun_request_t;
typedef struct stun_response_t stun_response_t;

enum { STUN_RFC_3489, STUN_RFC_5389, };

enum { STUN_PROTOCOL_UDP, STUN_PROTOCOL_TCP, STUN_PROTOCOL_TLS, };

/// @param[in] req transaction request
/// @param[in] resp transaction response, NULL if timeout
/// @param[in] code http like code, 2xx-ok, 4xx/5xx-error
/// @param[in] phrase error code phrase
/// @return 0-ok, other-error
typedef int (*stun_request_handler)(void* param, const stun_request_t* req, int code, const char* phrase);

/// @param[in] rfc STUN rfc version: STUN_RFC_3489/STUN_RFC_5389
stun_request_t* stun_request_create(stun_agent_t* stun, int rfc, stun_request_handler handler, void* param);

/// @param[in] protocol 1-UDP, 2-TCP
int stun_request_setaddr(stun_request_t* req, int protocol, const struct sockaddr* local, const struct sockaddr* remote);
int stun_request_getaddr(const stun_request_t* req, int* protocol, struct sockaddr_storage* local, struct sockaddr_storage* remote, struct sockaddr_storage* reflexive);

/// @param[in] credential 0-Short-Term Credential Mechanism, 1-Long-Term Credential Mechanism
/// @param[in] realm Long-Term Credential only
/// @param[in] nonce Long-Term Credential only
/// @return 0-ok, other-error
int stun_request_setauth(stun_request_t* req, int credential, const char* usr, const char* pwd, const char* realm, const char* nonce);
/// stun_agent_shared_secret response username/password
/// @return 0-ok, other-error
int stun_request_getauth(const stun_request_t* req, char usr[512], char pwd[512]);

struct stun_agent_handler_t
{
	/// @return 0-ok, other-error
	int (*send)(void* param, int protocol, const struct sockaddr* local, const struct sockaddr* remote, const void* data, int bytes);

	/// get pwd
	/// @param[in] cred STUN_CREDENTIAL_SHORT_TERM/STUN_CREDENTIAL_LONG_TERM
	/// @param[in] realm/nonce, if usr is null, return realm/nonce
	/// @param[out] pwd password of the usr
	/// @return 0-ok, other-error(update realm/nonce)
	int (*auth)(void* param, int cred, const char* usr, const char* realm, const char* nonce, char pwd[512]);
	
	/// turn long-term credential get realm/nonce
	int (*getnonce)(void* param, char realm[128], char nonce[128]);

	// stun
	int (*onbind)(void* param, stun_response_t* resp, const stun_request_t* req);
	int (*onsharedsecret)(void* param, stun_response_t* resp, const stun_request_t* req);
	int (*onindication)(void* param, stun_response_t* resp, const stun_request_t* req);

	// turn
	int (*onallocate)(void* param, stun_response_t* resp, const stun_request_t* req);
	int (*onrefresh)(void* param, stun_response_t* resp, const stun_request_t* req, int lifetime);
	int (*onpermission)(void* param, stun_response_t* resp, const stun_request_t* req, const struct sockaddr* peer);
	int (*onchannel)(void* param, stun_response_t* resp, const stun_request_t* req, const struct sockaddr* peer, uint16_t channel);
	int (*onsend)(void* param, stun_response_t* resp, const stun_request_t* req, const struct sockaddr* peer, const void* data, int bytes);
};

stun_agent_t* stun_agent_create(int rfc, struct stun_agent_handler_t* handler, void* param);
int stun_agent_destroy(stun_agent_t** stun);

int stun_agent_input(stun_agent_t* stun, int protocol, const struct sockaddr* local, const struct sockaddr* remote, const void* data, int bytes);

// STUN
int stun_agent_bind(stun_request_t* req);
int stun_agent_shared_secret(stun_request_t* req);

/// TURN data callback
/// @param[in] param user-defined parameter form turn_agent_allocate
typedef void (*turn_agent_ondata)(void* param, const void* data, int byte, int protocol, const struct sockaddr* local, const struct sockaddr* remote);

// TURN
int turn_agent_allocate(stun_request_t* req, turn_agent_ondata ondata, void* param);
int turn_agent_refresh(stun_request_t* req, int expired);
int turn_agent_create_permission(stun_request_t* req, const struct sockaddr* peer);
/// @param[in] channel valid range: [0x4000, 0x7FFE]
int turn_agent_channel_bind(stun_request_t* req, const struct sockaddr* peer, uint16_t channel);
/// Send data from client to turn server(and forward to peer)
int turn_agent_send(stun_request_t* req, const struct sockaddr* peer, const void* data, int bytes);

// RESPONSE

/// ignore request
int stun_agent_discard(struct stun_response_t* resp);

int stun_agent_bind_response(struct stun_response_t* resp, int code, const char* pharse);
int stun_agent_shared_secret_response(struct stun_response_t* resp, int code, const char* pharse, const char* usr, const char* pwd);
int turn_agent_allocate_response(struct stun_response_t* resp, const struct sockaddr* relay, int code, const char* pharse);
int turn_agent_refresh_response(struct stun_response_t* resp, int code, const char* pharse);
int turn_agent_create_permission_response(struct stun_response_t* resp, int code, const char* pharse);
int turn_agent_channel_bind_response(struct stun_response_t* resp, int code, const char* pharse);

void* stun_timer_start(int ms, void(*ontimer)(void* param), void* param);
int stun_timer_stop(void* timer);

#ifdef __cplusplus
}
#endif

#endif /* !_stun_agent_h_ */