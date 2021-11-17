#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>

#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <endian.h>
#include <netdb.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/signalfd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef __has_attribute
#	if __has_attribute(always_inline)
#		define always_inline static inline __attribute__((always_inline))
#	else
#		define always_inline static inline
#	endif
#	if __has_attribute(packed)
#		define packed __attribute__((packed))
#	else
#		define packed
#	endif
#else
#	define always_inline static inline
#	define packed
#endif

#define MAXEVS 32

typedef enum cnt_t
{
	CNT_UTF8 = 0x00,
	CNT_GEMTEXT = 0x01,
	CNT_ASCII = 0x02,
	CNT_RAW = 0x1f,
	CNT_REDIR = 0x20,
	CNT_EFILE = 0x22,
	CNT_ESRV = 0x23,
	CNT_VER = 0x24
} cnt_t;

typedef struct loop_t
{
	int efd;
	int xfd;
	bool run;
	struct epoll_event evs[MAXEVS];
} loop_t;

int loop_init(loop_t* loop);
int loop_add_fd(loop_t* loop, int fd);
int loop_add_sigs(loop_t* loop, ...);
void loop_destroy(loop_t* loop);

#define loop_run(loop, once, code) \
{ \
	int loop_i = 0; \
	int loop_fd = 0; \
	int loop_fdc = 0; \
	bool loop_once = false; \
	loop.run = true; \
	while (loop.run) \
	{ \
		if (!loop_once) \
		{ \
			loop_once = true; \
			once; \
		} \
		loop_fdc = epoll_wait(loop.efd, loop.evs, MAXEVS, -1); \
		if (loop_fdc < 0) \
		{ \
			break; \
		} \
		for (loop_i = 0; loop_i < loop_fdc; loop_i++) \
		{ \
			loop_fd = loop.evs[loop_i].data.fd; \
			code; \
		} \
	} \
}

#endif /* !COMMON_H */
