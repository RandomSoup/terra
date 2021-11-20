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
#define VERSION "v0.4.0"
#define LOADING "Loading..."
#define SURF \
	uint8_t bpp; /* Bytes per pixel, not bits */ \
	uint32_t width; \
	uint32_t height; \
	uint32_t stride; \
	uint64_t sz; \
	uint8_t* map;
#define PARENT "/../"
#define PARENT_SZ (sizeof(PARENT) - 1)

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

typedef struct piper_t
{
	int fd;
	int stage;
	size_t off;
	size_t rem;

	struct packed
	{
		char* url;
		char* uri;
		char* host;
		char* port;
	};

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
} piper_t;

typedef struct surf_t
{
	SURF
} surf_t;

typedef struct gui_t
{
	SURF

	int fd;
	int surfc;
	void* udata;

	GC gc;
	XIC xic;
	Window win;
	XImage* img;
	Display* dpy;
	Atom close_atom;

	surf_t* surfs;
	struct packed
	{
		void (*key)(int sym, void* udata);
		void (*utf8)(char* utf8, int len, void* udata);
		void (*click)(int button, int x, int y, void* udata);
	} cbs;
} gui_t;

typedef struct gem_t
{
	bool pre;
	char* str;
	char* url;
	el_t type;
} gem_t;

typedef struct cur_t
{
	el_t type;
	gem_t gem;
	char* str;
	bool is_gem;
	uint32_t off;
} cur_t;

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

typedef struct rover_t
{
	gui_t* gui;
	loop_t* loop;
	piper_t* piper;
	dynstr_t* inp;
	dynarr_t* lines;
	dynarr_t* links;
	cur_t* cur;
	bool pending;
	ssize_t lineno;
	char* url;
	const char* status;
} rover_t;

int piper_build(piper_t* piper, const char* url);
int piper_start(piper_t* piper);
void piper_handle(piper_t* piper);
void piper_free(piper_t* piper);

int gui_init(gui_t* gui);
bool gui_handle(gui_t* gui);
int gui_draw(gui_t* gui);
void gui_destroy(gui_t* gui);

#define draw_setpx(surf, x, y, color) (*(uint32_t*)((surf)->map + (x) * (surf)->bpp + (y) * (surf)->stride) = color)
void draw_fill(surf_t* surf, uint32_t color);
int draw_chr(surf_t* surf, char32_t chr, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);
void draw_str(surf_t* surf, char* str, uint64_t sz, int row, uint32_t color);
void draw_line(surf_t* surf, line_t* line, cur_t* cur, int row);
int draw_url(surf_t* surf, char* url);
int draw_status(surf_t* surf, const char* status);

void gem_parse(gem_t* dst, char* src);
int line_next(line_t* line, cur_t* cur);

void rover_render(rover_t* rover);
void rover_load(rover_t* rover);
void rover_utf8_cb(char* utf8, int len, void* udata);
void rover_key_cb(int key, void* udata);
void rover_click_cb(int button, int x, int y, void* udata);
char* rover_resolve_path(piper_t* piper, char* src, bool redir);

#endif /* !ROVER_H */
