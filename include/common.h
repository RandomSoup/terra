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

#ifndef FALSE
#	define FALSE 0
#endif

#ifndef TRUE
#	define TRUE !FALSE
#endif

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
	bool run;
	struct epoll_event evs[MAXEVS];
} loop_t;

typedef struct dynarr_t
{
	void* ptr;
	size_t sz; /* Total */
	ssize_t len; /* Used */
	size_t type;
} dynarr_t;

typedef struct dynstr_t
{
	char* ptr;
	size_t sz; /* Total */
	ssize_t len; /* Used */
} dynstr_t;

int loop_init(loop_t* loop);
int loop_add_fd(loop_t* loop, int fd);
int loop_add_sigs(loop_t* loop, ...);
void loop_destroy(loop_t* loop);

#define loop_run(loop, before, after) \
{ \
	int loop_i = 0; \
	int loop_fd = 0; \
	int loop_fdc = 0; \
	loop.run = true; \
	while (loop.run) \
	{ \
		before; \
		loop_fdc = epoll_wait(loop.efd, loop.evs, MAXEVS, -1); \
		if (loop_fdc < 0) \
		{ \
			break; \
		} \
		for (loop_i = 0; loop_i < loop_fdc; loop_i++) \
		{ \
			loop_fd = loop.evs[loop_i].data.fd; \
			after; \
		} \
	} \
}

int _dynarr_init(dynarr_t* arr, size_t sz, size_t type);
#define dynarr_init(arr, sz, type) _dynarr_init(arr, sz, sizeof(type))
int dynarr_add(dynarr_t* arr, void* ptr);
void* dynarr_get(dynarr_t* arr, size_t i);
void dynarr_free(dynarr_t* arr);
void dynarr_reset(dynarr_t* arr);

int dynstr_init(dynstr_t* str, size_t sz);
int dynstr_alloc(dynstr_t* str, size_t sz);
int dynstr_set(dynstr_t* str, char* ptr);
void dynstr_free(dynstr_t* str);

/* Utility functions */
size_t strsubs(const char* str, const char* sub);
const char* strskip(const char* str, const char* sub);
char* strdel(char* str, char chr);
int set_nonblock(int fd);

#endif /* !COMMON_H */
