#ifndef ROVER_H
#define ROVER_H

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
#include <uchar.h>
#include <time.h>
#include <ctype.h>
#include <netdb.h>
#include <endian.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <linux/sockios.h>
#include <threads.h>
#include <errno.h>

#include <epoxy/gl.h>
#include <GLFW/glfw3.h>

#define PIPER "piper://"
#define ABOUT "about:"
#define SURFW MAXC * FONTW + XOFF * 2
#define MAXEVS 20
#define VERSION "v0.2.0"

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

typedef struct surf_t
{
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint64_t sz;
	uint8_t* map;
	uint8_t bpp; /* Bytes per pixel, not bits */
} surf_t;

typedef struct dynarr_t
{
	void* ptr;
	size_t sz; /* Total */
	size_t len; /* Used */
	size_t type;
} dynarr_t;

typedef struct dynstr_t
{
	char* ptr;
	ssize_t sz; /* Total */
	ssize_t len; /* Used */
} dynstr_t;

typedef struct packed line_t
{
	uint32_t off : 29;
	el_t type : 4;
	uint8_t sz : 7;
} line_t;

typedef struct packed link_t
{
	char* url;
	uint32_t line;
} link_t;

typedef struct gem_t
{
	bool pre;
	el_t type;
	char* str;
	char* url;
	char* rnt;
} gem_t;

typedef struct cur_t
{
	char* str;
	uint32_t off;
	bool is_gem;
	el_t type;
	gem_t gem;
} cur_t;

typedef struct req_t
{
	char* url;
	char* uri;
	char* host;
	char* port;
	bool first;
} req_t;

typedef struct res_t
{
	union
	{
		struct packed
		{
			uint8_t type;
			uint64_t sz;
		};
		uint8_t hdr[sizeof(uint64_t) + 1];
	};
	char* buff;
} res_t;

typedef struct state_t
{
	cur_t* cur;
	surf_t* surfs;
	dynarr_t* arr;
	dynarr_t* links;
	size_t i;
	req_t* req;
	res_t* res;
	char* url;
	dynstr_t* inp;
	const char* status;
} state_t;

typedef struct loop_t
{
	int efd;
	bool run;
	struct epoll_event evs[MAXEVS];
} loop_t;

int draw_chr(surf_t* surf, char32_t chr, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);
void draw_fill(surf_t* surf, uint32_t color);
int draw_str(surf_t* surf, int row, char* str, uint64_t sz, uint32_t color);
int draw_line(surf_t* surf, int row, cur_t* cur, line_t* line);
int draw_url(surf_t* surf, char* url);
int draw_status(surf_t* surf, const char* status);

int _dynarr_init(dynarr_t* arr, size_t sz, size_t type);
#define dynarr_init(arr, sz, type) _dynarr_init(arr, sz, sizeof(type))
int dynarr_add(dynarr_t* arr, void* ptr);
void* dynarr_get(dynarr_t* arr, size_t i);
void dynarr_free(dynarr_t* arr);
void dynarr_reset(dynarr_t* arr);

int line_next(line_t* line, cur_t* cur);

int piper_build(req_t* req, res_t* res, const char* url);
int piper_start(res_t* res, req_t* req);
void piper_free(res_t* res, req_t* req);

void gem_parse(gem_t* dst, char* src);

int dynstr_init(dynstr_t* str, size_t sz);
int dynstr_alloc(dynstr_t* str, ssize_t sz);
int dynstr_set(dynstr_t* str, char* ptr);
void dynstr_free(dynstr_t* str);

int loop_init(loop_t* loop);
int loop_add_fd(loop_t* loop, int fd);
void loop_destroy(loop_t* loop);

#define loop_run(loop, code) \
{ \
	int loop_fd; \
	int loop_fdc; \
	loop.run = true; \
	while (loop.run) \
	{ \
		loop_fdc = epoll_wait(loop.efd, loop.evs, MAXEVS, -1); \
		if (loop_fdc < 0) \
		{ \
			break; \
		} \
		for (int loop_i = 0; loop_i < loop_fdc; loop_i++) \
		{ \
			loop_fd = loop.evs[loop_i].data.fd; \
			code; \
		} \
	} \
}

int worker_thread(void* udata);

extern int ps[2];

#endif /* !WWW_H */
