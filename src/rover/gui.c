#include "rover.h"
#include "config.h"

int gui_init(gui_t* gui)
{
	int fd;
	int surfc;
	uint64_t sz = 0;

	XIM xim;
	XIC xic;
	Window win;
	Window root;
	XImage* img;
	Display* dpy;
	Visual* visual;

	surf_t* surfs;

	surfs = gui->surfs;
	surfc = gui->surfc;
	gui->stride = gui->width * gui->bpp;
	gui->height = 0;

	for (int i = 0; i < surfc; i++)
	{
		surfs[i].bpp = gui->bpp;
		surfs[i].width = gui->width;
		surfs[i].stride = gui->stride;
		surfs[i].sz = surfs[i].height * gui->stride;
		gui->height += surfs[i].height;
	}
	gui->sz = gui->height * gui->stride;

	gui->map = malloc(gui->sz);
	if (!gui->map)
	{
		return -1;
	}

	for (int i = 0; i < surfc; i++)
	{
		surfs[i].map = gui->map + sz;
		sz += surfs[i].sz;
	}

	dpy = XOpenDisplay(NULL);
	fd = ConnectionNumber(dpy);
	root = RootWindow(dpy, 0);
	win = XCreateSimpleWindow(dpy, root, 0, 0, gui->width, gui->height, 0, 0, 0);
	visual = DefaultVisual(dpy, 0);
	img = XCreateImage(dpy, visual, DefaultDepth(dpy, 0), ZPixmap, 0, (char*)gui->map, gui->width, gui->height, 32, 0);

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
	gui->fd = fd;
	return fd;
}

bool gui_handle(gui_t* gui)
{
	int x, y;
	int tmp_x, tmp_y;
	int rt = 0;
	unsigned int mask;
	char utf8[MB_CUR_MAX + 1];

	Window tmp;
	Status xst;
	Display* dpy;
	KeySym sym = NoSymbol;
	XEvent ev = { 0x00 };

	dpy = gui->dpy;
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
				if (sym < 0xff00 && (xst == XLookupChars || xst == XLookupBoth) && gui->cbs.utf8)
				{
					gui->cbs.utf8(utf8, rt, gui->udata);
				} else if (gui->cbs.key)
				{
					gui->cbs.key(sym, gui->udata);
				}
			break;
			case ButtonRelease:
				if (gui->cbs.click)
				{
					XQueryPointer(dpy, gui->win, &tmp, &tmp, &tmp_x, &tmp_y, &x, &y, &mask);
					gui->cbs.click(ev.xbutton.button, x, y, gui->udata);
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

int gui_draw(gui_t* gui)
{
	return XPutImage(gui->dpy, gui->win, gui->gc, gui->img, 0, 0, 0, 0, gui->width, gui->height);
}

void gui_destroy(gui_t* gui)
{
	XDestroyImage(gui->img);
	XCloseDisplay(gui->dpy);
	return;
}
