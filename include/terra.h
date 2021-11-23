#ifndef TERRA_H
#define TERRA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/epoll.h>

#define TR_MAXEVS 32

#ifdef __has_attribute
#	if __has_attribute(always_inline)
#		define tr_always_inline static inline __attribute__((always_inline))
#	else
#		define tr_always_inline static inline
#	endif
#	if __has_attribute(packed)
#		define tr_packed __attribute__((packed))
#	else
#		define tr_packed
#	endif
#else
#	define tr_always_inline static inline
#	define tr_packed
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
	struct epoll_event evs[TR_MAXEVS];
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

typedef struct piper_t
{
	int fd;
	int stage;
	size_t off;
	size_t rem;

	struct tr_packed
	{
		char* url;
		char* uri;
		char* host;
		char* port;
	};

	union
	{
		struct tr_packed
		{
			uint8_t type;
			uint64_t sz;
		};
		uint8_t hdr[sizeof(uint64_t) + 1];
	};
	char* buff;
} piper_t;

typedef struct pget_t
{
	piper_t piper;
	loop_t loop;
	void* udata;
} pget_t;

typedef enum el_t
{
	EL_H1,
	EL_H2,
	EL_H3,
	EL_PRE,
	EL_TEXT,
	EL_LINK,
	EL_ITEM,
	EL_QUOTE,
	EL_IGNORE
} el_t;

typedef struct gem_t
{
	bool pre;
	char* str;
	char* url;
	el_t type;
} gem_t;

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
	(void)loop_i; (void)loop_fd; (void)loop_fdc; \
	while (loop.run) \
	{ \
		before; \
		loop_fdc = epoll_wait(loop.efd, loop.evs, TR_MAXEVS, -1); \
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

int piper_build(piper_t* piper, const char* url);
int piper_start(piper_t* piper);
void piper_handle(piper_t* piper);
void piper_free(piper_t* piper);
char* piper_path_resolve(piper_t* piper, char* src, bool redir);

pget_t* pget_easy_new();
int pget_easy_start(pget_t* pget, const char* url);
int pget_easy_get(pget_t* pget);
void pget_easy_free(pget_t* pget);

void gem_parse(gem_t* dst, char* src);

/* Utility functions */
size_t strsubs(const char* str, const char* sub);
const char* strskip(const char* str, const char* sub);
char* strdel(char* str, char chr);
int set_nonblock(int fd);

#endif /* !TERRA_H */
