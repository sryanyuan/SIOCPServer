#ifndef _INC_DEF_
#define _INC_DEF_
//////////////////////////////////////////////////////////////////////////
#define MAX_WORKER_THREAD_COUNT		20
#define MAX_SERVER_CONN				100
#define PACKET_HEADER_LENGTH		4
#define MAX_SEND_LENGTH_PER_TIME	(10 * 1024)

typedef void(*FUNC_ONACCEPT)(unsigned int);
typedef void(*FUNC_ONDISCONNECT)(unsigned int);
typedef void(*FUNC_ONRECV)(unsigned int, const char*, size_t);
//////////////////////////////////////////////////////////////////////////
#endif