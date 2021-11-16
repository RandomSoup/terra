#include "rover.h"
#define DEF_ABOUT
#include "config.h"

static const char* srv_msgs[] = { "File Not Found", "Internal Server Error" };
static const char* cli_msgs[] = {
	"Couldn't open socket", "Invalid address", "Connection timed out"
};

static void handle_doc(state_t* st)
{
	int i = 0;
	line_t line;
	link_t link;
	surf_t* surf;
	res_t* res;
	uint8_t type;

	surf = &st->surfs[2];
	dynstr_set(st->inp, st->url);
	draw_url(&st->surfs[3], st->inp->ptr);
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
	glfwPostEmptyEvent();
	return;
}

int worker_thread(void* udata)
{
	int stage = 0;
	int server = 0;
	size_t off = 0;
	size_t rem = 0;
	ssize_t rt;
	uint8_t op;
	void* tmp;

	loop_t loop;
	state_t* st;
	res_t* res;
	req_t* req;

	st = udata;
	res = st->res;
	req = st->req;
	loop_init(&loop);

	loop_add_fd(&loop, ps[0]);
	loop_run(loop, {
		if (loop_fd == ps[0])
		{
			rt = read(ps[0], &op, sizeof(op));
			if (!rt)
			{
				loop.run = false;
			} else
			{
start_req:
				off = 0;
				stage = 0;
				if (server > 0)
				{
					close(server);
				}
				server = 0;
				piper_free(res, req);

				st->status = "Loading...";
				if (!strncmp(st->url, ABOUT, sizeof(ABOUT) - 1))
				{
					res->buff = strdup(about);
					server = -1;
					handle_doc(st);
				} else if (piper_build(req, res, st->url))
				{
					st->status = "Invalid URL";
				} else
				{
					server = piper_start(res, req);
					if (server < 0)
					{
						res->type = server;
						handle_doc(st);
					} else
					{
						loop_add_fd(&loop, server);
					}
				}
			}
		} else
		{
			switch (stage)
			{
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
					rt = read(server, res->hdr + stage, 9 - stage);
					stage += rt;
					if (rt <= 0)
					{
						goto close_stage;
					}
					if (res->type == CNT_EFILE || res->type == CNT_ESRV)
					{
						handle_doc(st);
						goto close_stage;
					}
				break;
				case 9:
					res->sz = le64toh(res->sz);
					res->buff = malloc(res->sz + 1);
					if (!res->buff)
					{
						goto close_stage;
					}
					if (res->sz >= UINT64_MAX)
					{
						stage++;
					}
					stage++;
				break;
				case 10:
					rt = read(server, res->buff + off, res->sz - off);
					off += rt;
					if (rt < 0)
					{
						goto close_stage;
					} else if (off == res->sz || rt == 0)
					{
handle_res:
						res->buff[off] = 0x00;
						/* Basic anti-loop */
						if (res->type == CNT_REDIR && strcmp(res->buff, st->url))
						{
							free(st->url);
							st->url = strdup(res->buff);
							goto start_req;
						} else
						{
							handle_doc(st);
						}
						goto close_stage;
					}
				break;
				case 11:
					if (ioctl(server, SIOCINQ, &rem))
					{
						goto close_stage;
					}
					tmp = realloc(res->buff, off + rem);
					if (!tmp)
					{
						free(res->buff);
						goto close_stage;
					}
					res->buff = tmp;
					rt = read(server, res->buff + off, rem);
					off += rt;
					if (rt < 0)
					{
						goto close_stage;
					} else if (rt == 0)
					{
						goto handle_res;
					}
				break;
				default:
close_stage:
					stage = 0;
					close(server);
					server = 0;
				break;
			}
		}
	});
	loop_destroy(&loop);
	close(ps[0]);
	return 0;
}
