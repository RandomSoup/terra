#include "rover.h"
#define DEF_ABOUT
#include "config.h"

int main(int argc, char* argv[])
{
	int rt = 0;
	gui_t gui;
	loop_t loop;
	req_t req = { 0x00 };
	res_t res = { 0x00 };
	cur_t cur = { .is_gem = false };
	dynarr_t arr;
	dynarr_t links;
	dynstr_t inp;
	surf_t surfs[] = {
		{
			.bpp = 4,
			.width = SURFW,
			.height = 0,
			.sz = 0
		},
		{
			.bpp = 4,
			.width = SURFW,
			.height = FONTH + YOFF * 2
		},
		{
			.bpp = 4,
			.width = SURFW,
			.height = MAXR * FONTH + YOFF * 2
		},
		{
			.bpp = 4,
			.width = SURFW,
			.height = FONTH + YOFF * 2
		}
	};
	state_t st = { &req, &res, &cur, surfs, &arr, &inp, &links, 0, NULL, false, "Loading..." };
	conn_t conn = {
		.req = &req,
		.res = &res
	};

	dynarr_init(&arr, 16, line_t);
	dynarr_init(&links, 16, link_t);
	dynstr_init(&inp, MAXC);
	loop_init(&loop);

	loop.xfd = gui_init(&st, &gui, sizeof(surfs) / sizeof(*surfs));
	if (loop.xfd < 0)
	{
		goto end;
	}

	loop_add_fd(&loop, loop.xfd);
	st.url = strdup("about:");
	gui_handle(&st, &gui);
	loop_run(loop, {
		goto start_req;
	}, {
		if (loop_fd == loop.xfd)
		{
			loop.run = gui_handle(&st, &gui);
		} else
		{
			conn_handle(&st, &conn);
		}
		XPutImage(gui.dpy, gui.win, gui.gc, gui.img, 0, 0, 0, 0, surfs->width, surfs->height);
		if (st.pending)
		{
start_req:
			if (!st.url)
			{
				st.url = strdup(inp.ptr);
			}
			conn.off = 0;
			conn.stage = 0;
			st.pending = false;
			if (conn.server > 0)
			{
				close(conn.server);
			}
			conn.server = 0;
			piper_free(&res, &req);
			st.status = "Loading...";
			if (!strncmp(st.url, ABOUT, sizeof(ABOUT) - 1))
			{
				res.buff = strdup(about);
				conn.server = -1;
				gui_render(&st);
			} else if (piper_build(&req, &res, st.url))
			{
				st.status = "Invalid URL";
			} else
			{
				conn.server = piper_start(&res, &req);
				if (conn.server < 0)
				{
					res.type = conn.server;
					gui_render(&st);
				} else
				{
					loop_add_fd(&loop, conn.server);
				}
			}
		}
	});
	rt = 0;

end:
	loop_destroy(&loop);
	dynarr_free(&arr);
	dynarr_free(&links);
	dynstr_free(&inp);
	piper_free(&res, &req);
	if (conn.server > 0)
	{
		close(conn.server);
	}
	if (st.url)
	{
		free(st.url);
	}
	XDestroyImage(gui.img);
	XCloseDisplay(gui.dpy);
	return rt;
}
