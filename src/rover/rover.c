#include "rover.h"
#define DEF_ABOUT
#include "config.h"

static const char* srv_msgs[] = { "File Not Found", "Internal Server Error" };
static const char* cli_msgs[] = {
	"Couldn't open socket", "Invalid address", "Connection timed out"
};

always_inline void rover_set_status(rover_t* rover)
{
	uint8_t type;

	type = rover->piper->type;
	switch (type)
	{
		case CNT_UTF8:
		case CNT_ASCII:
			rover->status = "Text";
		break;
		case CNT_GEMTEXT:
			rover->status = "Gemtext";
		break;
		case CNT_EFILE:
		case CNT_ESRV:
			rover->status = srv_msgs[type - CNT_EFILE];
		break;
		case CNT_REDIR:
			rover->status = "Redirect Loop";
		break;
		default:
			/* Yes, magic number, I know. */
			if (type >= 0xff - 3)
			{
				rover->status = cli_msgs[0xff - type];
			} else
			{
				rover->status = "Not Implemented";
			}
		break;
	}
	return;
}

always_inline void rover_scroll(rover_t* rover, int key)
{
	size_t sz;
	size_t off;
	surf_t* surf;
	line_t* line;

	surf = &rover->gui->surfs[1];
	sz = FONTH * surf->stride;
	off = YOFF * surf->stride;
	if (key == XK_Up && rover->lineno > 0)
	{
		rover->lineno--;
		for (int i = MAXR - 1; i; i--)
		{
			memcpy(
				surf->map + off + i * sz,
				surf->map + off + (i - 1) * sz,
				sz
			);
		}
		line = dynarr_get(rover->lines, rover->lineno);
		draw_line(surf, line, rover->cur, 0);
	} else if (key == XK_Down && rover->lineno + MAXR < rover->lines->len)
	{
		rover->lineno++;
		for (int i = MAXR; i > 1; i--)
		{
			memcpy(
				surf->map + off + (MAXR - i) * sz,
				surf->map + off + ((MAXR - i) + 1) * sz,
				sz
			);
		}
		line = dynarr_get(rover->lines, rover->lineno + MAXR - 1);
		if (line)
		{
			draw_line(surf, line, rover->cur, MAXR - 1);
		}
	}
	return;
}

void rover_render(rover_t* rover)
{
	int i = 0;
	line_t line;
	link_t link;
	surf_t* surf;
	piper_t* piper;

	surf = &rover->gui->surfs[1];
	draw_fill(surf, BGCOLOR);
	draw_url(&rover->gui->surfs[0], rover->url);

	dynstr_set(rover->inp, rover->url);
	dynarr_reset(rover->lines);
	dynarr_reset(rover->links);

	piper = rover->piper;
	rover_set_status(rover);
	if (!piper->buff)
	{
		return;
	}

	memset(rover->cur, 0x00, sizeof(*rover->cur));
	rover->cur->type = EL_IGNORE;
	rover->cur->str = piper->buff;
	rover->cur->is_gem = piper->type == CNT_GEMTEXT;
	while (!line_next(&line, rover->cur))
	{
		if (line.type != EL_IGNORE)
		{
			if (line.type == EL_LINK)
			{
				link.url = rover->cur->gem.url;
				link.line = rover->lines->len;
				dynarr_add(rover->links, &link);
			}
			dynarr_add(rover->lines, &line);
			if (i < MAXR)
			{
				draw_line(surf, &line, rover->cur, i);
			}
		}
		i++;
	}
	return;
}

void rover_load(rover_t* rover)
{
	piper_t* piper;

	piper = rover->piper;
	if (!rover->url)
	{
		rover->url = strdup(rover->inp->ptr);
	}
	piper->off = 0;
	piper->stage = 0;
	rover->pending = false;
	piper_free(piper);
	rover->status = LOADING;
	if (strsubs(rover->url, ABOUT))
	{
		piper->buff = strdup(about);
		piper->fd = -1;
		rover_render(rover);
	} else if (piper_build(piper, rover->url))
	{
		rover->status = "Invalid URL";
	} else
	{
		piper->fd = piper_start(piper);
		if (piper->fd < 0)
		{
			piper->type = piper->fd;
			rover_render(rover);
		} else
		{
			loop_add_fd(rover->loop, piper->fd);
		}
	}
	return;
}

void rover_click_cb(int button, int x, int y, void* udata)
{
	uint32_t row, col;
	rover_t* rover;
	link_t* link;
	line_t* line;
	dynarr_t* links;

	if (button != Button1)
	{
		return;
	}
	rover = udata;

	row = (uint32_t)((y - (FONTH + YOFF * 3)) / FONTH);
	if (row > MAXR)
	{
		return;
	}
	row += rover->lineno;
	col = (uint32_t)((x - XOFF) / FONTW);
	line = dynarr_get(rover->lines, row);
	if (!line || col > line->sz)
	{
		return;
	}
	links = rover->links;
	for (ssize_t i = 0; i < links->len; i++)
	{
		link = dynarr_get(links, i);
		if (link->line == row)
		{
			rover->url = rover_resolve_path(rover->piper, link->url);
			rover->pending = true;
			break;
		}
	}
	return;
}

void rover_utf8_cb(char* utf8, int len, void* udata)
{
	rover_t* rover;
	dynstr_t* inp;

	rover = udata;
	inp = rover->inp;
	if (dynstr_alloc(inp, inp->len + len))
	{
		return;
	}
	memcpy(inp->ptr + inp->len, utf8, len);
	inp->len += len;
	inp->ptr[inp->len] = 0x00;
	draw_url(&rover->gui->surfs[0], rover->inp->ptr);
	return;
}

void rover_key_cb(int sym, void* udata)
{
	rover_t* rover;
	dynstr_t* inp;
	surf_t* surf;

	rover = udata;
	inp = rover->inp;
	surf = &rover->gui->surfs[0];
	switch (sym)
	{
		case XK_BackSpace:
			if (inp->len - 1 >= 0)
			{
				/* TODO: Handle multi-byte characters properly */
				inp->ptr[--inp->len] = 0x00;
			}
			draw_url(surf, inp->ptr);
		break;
		case XK_Up:
		case XK_Down:
			rover_scroll(rover, sym);
		break;
		case XK_Return:
			free(rover->url);
			rover->url = NULL;
			/* Fall through */
		case XK_F5:
			rover->pending = true;
		break;
	}
	return;
}

/*
 * Stolen^WAdapted from libpiper's `piper_resolve_url`
 * https://github.com/RandomSoup/libpiper/blob/5fcee501a7c20abeb3ac2a745da6d733fbfc64ee/src/common.c#L153
 * 
 */
char* rover_resolve_path(piper_t* piper, char* target)
{
	size_t i;
	size_t uri_sz;
	size_t target_sz;
	size_t last_slash = 0;
	char* rt;
	char* old_uri;
	char* new_uri;

	if (strsubs(target, PIPER))
	{
		rt = strdup(target);
	} else if (*target == '/')
	{
		asprintf(&rt, "%s:%s%s", piper->host, piper->port, target);
	} else
	{
		old_uri = piper->uri;
		uri_sz = strlen(old_uri);
		for (size_t j = 0; j < uri_sz; j++)
		{
			i = uri_sz - uri_sz - 1;
			if (old_uri[i] == '/')
			{
				last_slash = i;
				break;
			}
		}

		target_sz = strlen(target);
		uri_sz = last_slash + target_sz;

		new_uri = malloc(uri_sz + 1);
		if (!new_uri)
		{
			return NULL;
		}
		memcpy(new_uri, old_uri, last_slash);
		memcpy(&new_uri[last_slash], target, target_sz);
		new_uri[uri_sz] = 0x00;
		asprintf(&rt, "%s:%s/%s", piper->host, piper->port, new_uri);
		free(new_uri);
	}
	return rt;
}
