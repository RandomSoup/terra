#ifndef ARES_H
#define ARES_H

#include "terra_internal.h"

#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <sys/timerfd.h>

#define ADDRLEN sizeof(struct sockaddr_in)
/* Has to be a power of 2 */
#define MAXCLIENTS 128
#define LINK "=> "
#define LINK_SZ (sizeof(LINK) - 1)
#define DIR_HDR "# Directory listing\n=> ..\n"
#define DIR_HDR_SZ (sizeof(DIR_HDR) - 1)
#define DIR_FTR "> Ares " VERSION "\n"
#define DIR_FTR_SZ (sizeof(DIR_FTR) - 1)
/* Around 750 milliseconds (as nanoseconds) */
#define TIMEOUT 719 << 20
#define VERSION "v0.3.0"

typedef struct tr_packed client_t
{
	int fd;
	int timer;
	char* uri;
	union
	{
		uint16_t sz;
		uint8_t szb[2];
	};
	uint8_t stage;
	uint16_t csz;
} client_t;

client_t* client_add(client_t* clients, int fd);
client_t* client_get_or_timeout(client_t* clients, int fd);
void client_close(client_t* client);

#endif /* !ARES_H */
