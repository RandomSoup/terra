#ifndef ARES_H
#define ARES_H

#include "terra_internal.h"

#include <limits.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/wait.h>

#define ADDRLEN sizeof(struct sockaddr_in)
/* Has to be a power of 2 */
#define MAXCLIENTS 128
#define LINK "=> "
#define LINK_SZ (sizeof(LINK) - 1)
#define DIR_HDR "# Directory listing\n" LINK "..\n"
#define DIR_HDR_SZ (sizeof(DIR_HDR) - 1)
#define DIR_FTR "> Ares " VERSION "\n"
#define DIR_FTR_SZ (sizeof(DIR_FTR) - 1)
/* Around 750 milliseconds (as nanoseconds) */
#define TIMEOUT 719 << 20
#define VERSION "v0.4.0"
#define CFG_PATH "ares.cfg"

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

typedef struct cfg_t
{
	char* glob;
#define PROP(type, name) type name;
#include "ares_props.h"
#undef PROP
} cfg_t;

client_t* client_add(client_t* clients, int fd);
client_t* client_get_or_timeout(client_t* clients, int fd);
void client_close(client_t* client);

int cfg_load(dynarr_t* arr, char* path);
void cfg_get(cfg_t* cfg, cfg_t* cfgs, size_t sz, char* uri);
void cfg_free(dynarr_t* arr);

void mod_dir_handle(client_t* client, char* path);
void mod_cgi_handle(client_t* client, char* path);
void mod_file_handle(client_t* client, char* path, uint8_t type, struct stat* st);

#endif /* !ARES_H */
