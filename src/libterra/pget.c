#include "terra_internal.h"

pget_t* pget_easy_new()
{
	pget_t* pget;

	pget = malloc(sizeof(*pget));
	if (!pget)
	{
		return NULL;
	}
	loop_init(&pget->loop);
	return pget;
}

int pget_easy_start(pget_t* pget, const char* url)
{
	loop_t* loop;
	piper_t* piper;

	loop = &pget->loop;
	piper = &pget->piper;

	if (piper_build(piper, strdup(url)) || piper_start(piper) < 0)
	{
		return -1;
	}
	loop_add_fd(loop, piper->fd);
	return 0;
}

int pget_easy_get(pget_t* pget)
{
	char* url;
	int rt = 0;
	loop_t* loop;
	piper_t* piper;
	bool pending = false;

	loop = &pget->loop;
	piper = &pget->piper;

	loop_run((*loop), {
		if (pending)
		{
			pget_easy_start(pget, url);
			pending = false;
		}
	}, {
		piper_handle(piper);
		if (!piper->fd && piper->type == CNT_REDIR)
		{
			url = piper_path_resolve(piper, piper->buff, true);
			/* Basic anti-loop */
			if (strcmp(piper->url, url))
			{
				piper_free(piper);
				pending = true;
			} else
			{
				free(url);
				loop->run = false;
			}
		} else if (piper->fd <= 0)
		{
			rt = piper->fd;
			loop->run = false;
		}
	});
	return rt;
}

void pget_easy_free(pget_t* pget)
{
	loop_destroy(&pget->loop);
	piper_free(&pget->piper);
	memset(pget, 0x00, sizeof(*pget));
	free(pget);
	return;
}
