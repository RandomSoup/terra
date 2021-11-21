#ifndef ARES_H
#define ARES_H

#include "common.h"

#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/wait.h>

#define VERSION "v0.2.0"
#define ADDRLEN sizeof(struct sockaddr_in)
/* Has to be a power of 2 */
#define MAXCLIENTS 128
#define LINK "=> "
#define LINK_SZ (sizeof(LINK) - 1)
#define DIR_HDR "# Directory listing\n=> ..\n"
#define DIR_HDR_SZ (sizeof(DIR_HDR) - 1)
#define DIR_FTR "> Ares " VERSION "\n"
#define DIR_FTR_SZ (sizeof(DIR_FTR) - 1)

typedef struct packed client_t
{
	int fd;
	char* uri;
	union
	{
		uint16_t sz;
		uint8_t szb[2];
	};
	uint8_t stage;
	uint16_t csz;
} client_t;

int clients_set(client_t* clients, int fd);
client_t* clients_get(client_t* clients, int fd);
void clients_del(client_t* clients, int fd);

#endif /* !ARES_H */
