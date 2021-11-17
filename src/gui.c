#include "rover.h"
#include "config.h"

static const char* srv_msgs[] = { "File Not Found", "Internal Server Error" };
static const char* cli_msgs[] = {
	"Couldn't open socket", "Invalid address", "Connection timed out"
};

int gui_init(state_t* st, gui_t* gui, int surfc)
{
	int fd;
	int i = 1;
	uint64_t sz = 0;

	Display* dpy;
	Window win;
	Window root;
	XIM xim;
	XIC xic;
	XImage* img;
	Visual* visual;

	surf_t* surfs;

	surfs = st->surfs;
	surfs->stride = surfs->width * surfs->bpp;
	while (i < surfc)
	{
		surfs[i].stride = surfs[i].width * surfs[i].bpp;
		surfs[i].sz = surfs[i].height * surfs[i].stride;
		surfs->height += surfs[i].height;
		i++;
	}
	surfs->sz = surfs->height * surfs->stride;

	surfs->map = malloc(surfs->sz);
	if (!surfs->map)
	{
		return -1;
	}

	i = 1;
	sz = 0;
	while (i < surfc)
	{
		surfs[i].map = surfs->map + sz;
		sz += surfs[i].sz;
		i++;
	}
	draw_fill(surfs, BGCOLOR);

	dpy = XOpenDisplay(NULL);
	fd = ConnectionNumber(dpy);
	root = RootWindow(dpy, DefaultScreen(dpy));
	win = XCreateSimpleWindow(dpy, root, 0, 0, surfs->width, surfs->height, 0, 0, 0);
	visual = DefaultVisual(dpy, 0);
	img = XCreateImage(dpy, visual, DefaultDepth(dpy, 0), ZPixmap, 0, (char*)surfs->map, surfs->width, surfs->height, 32, 0);

	gui->close_atom = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XStoreName(dpy, win, "Rover");
	XSetWMProtocols(dpy, win, &gui->close_atom, 1);
	XMapRaised(dpy, win);

	XSetLocaleModifiers("");
	xim = XOpenIM(dpy, 0, 0, 0);
	if (!xim)
	{
		XSetLocaleModifiers("@im=none");
		xim = XOpenIM(dpy, 0, 0, 0);
	}

	xic = XCreateIC(xim,
			XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
			XNClientWindow, win, XNFocusWindow, win, NULL);
	XSetICFocus(xic);

	XSelectInput(dpy, win, KeyPressMask | KeyReleaseMask | ExposureMask | ButtonPressMask | ButtonReleaseMask);
	gui->dpy = dpy;
	gui->xic = xic;
	gui->win = win;
	gui->img = img;
	gui->gc = DefaultGC(dpy, 0);
	return fd;
}

void gui_render(state_t* st)
{
	int i = 0;
	line_t line;
	link_t link;
	surf_t* surf;
	res_t* res;
	uint8_t type;

	surf = &st->surfs[2];
	dynstr_set(st->inp, st->url);
	draw_url(&st->surfs[1], st->inp->ptr);
	draw_fill(&st->surfs[2], BGCOLOR);

	dynarr_reset(st->arr);
	dynarr_reset(st->links);

	res = st->res;
	if (!res->buff)
	{
		goto set_status;
	}

	memset(st->cur, 0x00, sizeof(*st->cur));
	st->cur->str = res->buff;
	st->cur->is_gem = res->type == CNT_GEMTEXT;
	while (!line_next(&line, st->cur))
	{
		if (line.type != EL_IGNORE)
		{
			if (line.type == EL_LINK)
			{
				link.url = st->cur->gem.url;
				link.line = st->arr->len;
				dynarr_add(st->links, &link);
			}
			dynarr_add(st->arr, &line);
		}
	}

	while (i < MAXR && i < (int)st->arr->len)
	{
		draw_line(surf, i, st->cur, dynarr_get(st->arr, i));
		i++;
	}

set_status:
	type = res->type;
	switch (type)
	{
		case CNT_UTF8:
		case CNT_ASCII:
			st->status = "Text";
		break;
		case CNT_GEMTEXT:
			st->status = "Gemtext";
		break;
		case CNT_EFILE:
		case CNT_ESRV:
			st->status = srv_msgs[type - CNT_EFILE];
		break;
		case CNT_REDIR:
			st->status = "Redirect Loop";
		break;
		default:
			/* Yes, magic number, I know. */
			if (type >= 0xff - 3)
			{
				st->status = cli_msgs[0xff - type];
			} else
			{
				st->status = "Not Implemented";
			}
		break;
	}
	draw_status(&st->surfs[3], st->status);
	return;
}

static void gui_scroll(state_t* st, int key)
{
	size_t sz;
	size_t off;
	surf_t* surf;
	line_t* line;

	surf = &st->surfs[2];
	sz = FONTH * surf->stride;
	off = YOFF * surf->stride;
	if (key == XK_Up && st->i > 0)
	{
		st->i--;
		memmove(surf->map + sz + off, surf->map + off, surf->sz - sz - off * 2);
		draw_line(surf, 0, st->cur, dynarr_get(st->arr, st->i));
	} else if (key == XK_Down && st->i + MAXR < st->arr->len)
	{
		st->i++;
		memmove(surf->map + off, surf->map + sz + off, surf->sz - sz - off * 2);
		if ((line = dynarr_get(st->arr, st->i + MAXR - 1)))
		{
			draw_line(surf, MAXR - 1, st->cur, line);
		}
	}
	return;
}

static void gui_click(state_t* st, int x, int y)
{
	size_t i = 0;
	uint32_t row, col;
	link_t* link;
	line_t* line;
	dynarr_t* links;

	row = (uint32_t)((y - (FONTH + YOFF * 3)) / FONTH);
	if (row > MAXR)
	{
		return;
	}
	row += st->i;
	col = (uint32_t)((x - XOFF) / FONTW);
	line = dynarr_get(st->arr, row);
	if (!line || col > line->sz)
	{
		return;
	}
	links = st->links;
	while (i < links->len)
	{
		link = dynarr_get(links, i);
		if (link->line == row)
		{
			free(st->url);
			st->pending = true;
			st->url = strdup(link->url);
			break;
		}
		i++;
	}
	return;
}

bool gui_handle(state_t* st, gui_t* gui)
{
	Window tmp;
	Status xst;
	Display* dpy;
	KeySym sym = NoSymbol;
	XEvent ev = { 0x00 };

	int x, y;
	int tmp_x, tmp_y;
	int rt = 0;
	unsigned int mask;
	char utf8[MB_CUR_MAX + 1];
	dynstr_t* inp;

	dpy = gui->dpy;
	inp = st->inp;
	while (XPending(dpy))
	{
		XNextEvent(dpy, &ev);
		if (XFilterEvent(&ev, None) == True)
		{
			continue;
		}
		switch (ev.type)
		{
			case KeyPress:
				ev.xkey.state &= ~ControlMask;
				rt = Xutf8LookupString(gui->xic, &ev.xkey, utf8, MB_CUR_MAX, &sym, &xst);
				if (sym < 0xff00 && (xst == XLookupChars || xst == XLookupBoth))
				{
					if (dynstr_alloc(inp, inp->len + rt))
					{
						return -1;
					}
					memcpy(inp->ptr + inp->len, utf8, rt);
					inp->len += rt;
					inp->ptr[inp->len] = 0x00;
					draw_url(&st->surfs[1], inp->ptr);
				} else
				{
					switch (sym)
					{
						case XK_BackSpace:
							if (inp->len - 1 >= 0)
							{
								/* TODO: Handle multi-byte characters properly */
								inp->ptr[--inp->len] = 0x00;
							}
							draw_url(&st->surfs[1], inp->ptr);
						break;
						break;
						case XK_Up:
						case XK_Down:
							gui_scroll(st, sym);
						break;
						case XK_Return:
							free(st->url);
							st->url = NULL;
							/* Fall through */
						case XK_F5:
							st->pending = true;
						break;
						case XK_Escape:
							return false;
						break;
					}
				}
			break;
			case ButtonRelease:
				if (ev.xbutton.button == Button1)
				{
					XQueryPointer(dpy, gui->win, &tmp, &tmp, &tmp_x, &tmp_y, &x, &y, &mask);
					gui_click(st, x, y);
				}
			break;	
			case ClientMessage:
				if ((Atom)ev.xclient.data.l[0] == gui->close_atom)
				{
					return false;
				}
			break;
			case Expose:
				/* We handle this in the main loop */
			default:
			break;
		}
	}
	return true;
}
