#ifndef ROVER_H
#define ROVER_H

#include "common.h"

#include <uchar.h>
#include <time.h>
#include <ctype.h>

#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <linux/sockios.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define PIPER "piper://"
#define ABOUT "about:"
#define SURFW (MAXC * FONTW + XOFF * 2)
#define VERSION "v0.3.0"

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
	req_t* req;
	res_t* res;
	cur_t* cur;
	surf_t* surfs;
	dynarr_t* arr;
	dynstr_t* inp;
	dynarr_t* links;
	size_t i;
	char* url;
	bool pending;
	const char* status;
} state_t;

typedef struct conn_t
{
	req_t* req;
	res_t* res;
	int server;
	int stage;
	size_t off;
	size_t rem;
} conn_t;

typedef struct gui_t
{
	GC gc;
	XIC xic;
	Window win;
	XImage* img;
	Display* dpy;
	Atom close_atom;
} gui_t;

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

int gui_init(state_t* st, gui_t* gui, int surfc);
void gui_render(state_t* st);
bool gui_handle(state_t* st, gui_t* gui);

void conn_handle(state_t* st, conn_t* conn);

#endif /* !ROVER_H */
