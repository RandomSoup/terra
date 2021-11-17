#include "rover.h"
#include "config.h"

#include "cwalk.h"

void conn_handle(state_t* st, conn_t* conn)
{
	int stage;
	int server;
	void* tmp;
	ssize_t rt;
	char redir[UINT16_MAX + 8];
	req_t* req;
	res_t* res;

	server = conn->server;
	stage = conn->stage;
	req = conn->req;
	res = conn->res;
	switch (conn->stage)
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
				gui_render(st);
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
			rt = read(server, res->buff + conn->off, res->sz - conn->off);
			conn->off += rt;
			if (rt < 0)
			{
				goto close_stage;
			}
			if (conn->off == res->sz || rt == 0)
			{
handle_res:
				res->buff[conn->off] = 0x00;
				/* Basic anti-loop */
				if (res->type == CNT_REDIR && strcmp(res->buff, st->url))
				{
					if (strncmp(res->buff, PIPER, sizeof(PIPER) - 1))
					{
						sprintf(redir, "%s/..", req->uri);
						cwk_path_get_absolute(redir, res->buff, redir, UINT16_MAX + 1);
						free(st->url);
						asprintf(&st->url, "%s:%s%s", req->host, req->port, redir);
					} else
					{
						free(st->url);
						st->url = strdup(res->buff);
					}
					st->pending = true;
				} else
				{
					gui_render(st);
				}
				goto close_stage;
			}
		break;
		case 11:
			if (ioctl(server, SIOCINQ, &conn->rem))
			{
				goto close_stage;
			}
			tmp = realloc(res->buff, conn->off + conn->rem);
			if (!tmp)
			{
				free(res->buff);
				goto close_stage;
			}
			res->buff = tmp;
			rt = read(server, res->buff + conn->off, conn->rem);
			conn->off += rt;
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
			conn->stage = 0;
			close(server);
			conn->server = 0;
		break;
	}
	conn->stage = stage;
	conn->server = server;
	return;
}
