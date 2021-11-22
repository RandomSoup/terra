#include "rover.h"
#include "config.h"

int main(int argc, char* argv[])
{
	char* tmp;
	gui_t gui;
	loop_t loop;
	surf_t surfs[] = {
		{
			.height = FONTH + YOFF * 2
		},
		{
			.height = MAXR * FONTH + YOFF * 2
		},
		{
			.height = FONTH + YOFF * 2
		}
	};
	dynstr_t url;
	dynarr_t lines;
	dynarr_t links;
	piper_t piper = { 0x00 };
	cur_t cur = { .is_gem = false };
	rover_t rover = { 
		&gui, &loop, &piper, &url, &lines, &links, &cur, false, 0, NULL, LOADING
	};

	dynarr_init(&lines, 16, line_t);
	dynarr_init(&links, 16, link_t);
	dynstr_init(&url, MAXC);

	rover.url = strdup("about:");
	rover.pending = true;

	gui.bpp = 4;
	gui.width = MAXC * FONTW + XOFF * 2;
	gui.surfc = 3;
	gui.surfs = surfs;
	gui.udata = &rover;

	gui.cbs.click = rover_click_cb;
	gui.cbs.utf8 = rover_utf8_cb;
	gui.cbs.key = rover_key_cb;

	gui_init(&gui);
	draw_fill((surf_t*)&gui, BGCOLOR);

	loop_init(&loop);
	loop_add_fd(&loop, gui.fd);
	loop_run(loop, {
		if (rover.pending)
		{
			rover_load(&rover);
		}
		draw_status(&surfs[2], rover.status);
		gui_draw(&gui);
	}, {
		if (loop_fd == piper.fd)
		{
			piper_handle(&piper);
			if (piper.fd <= 0)
			{
				if (piper.type == CNT_REDIR)
				{
					tmp = piper_path_resolve(&piper, piper.buff, true);
					/* Basic anti-loop */
					if (strcmp(rover.url, tmp))
					{
						free(rover.url);
						rover.url = tmp;
						rover.pending = true;
					} else
					{
						free(tmp);
						rover_render(&rover);
					}
				} else
				{
					rover_render(&rover);
				}
			}
		} else if (loop_fd == gui.fd)
		{
			loop.run = gui_handle(&gui);
		}
	});

	free(rover.url);
	dynarr_free(&lines);
	dynarr_free(&links);
	loop_destroy(&loop);
	gui_destroy(&gui);
	piper_free(&piper);
	dynstr_free(&url);
	return 0;
}
