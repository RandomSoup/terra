#include "rover.h"
#include "config.h"

int ps[2];

static int load_page(state_t* st, char* url)
{
	uint8_t tmp = 0x00;

	if (!url)
	{
		url = st->url;
	} else if (st->url)
	{
		free(st->url);
	}
	st->url = url;
	write(ps[1], &tmp, sizeof(tmp));
	return 0;
}

static void handle_scroll(state_t* st, int key)
{
	int i;
	void* dst;
	void* src;
	size_t sz;
	surf_t* surf;
	line_t* line;

	surf = &st->surfs[2];
	sz = FONTH * surf->stride;
	if (key == GLFW_KEY_UP && st->i > 0)
	{
		i = MAXR - 1;
		while (i)
		{
			dst = surf->map + surf->sz - ((YOFF + (i + 1) * FONTH) * surf->stride);
			src = surf->map + (YOFF + (MAXR - i) * FONTH) * surf->stride;
			memcpy(dst, src, sz);
			i--;
		}
		st->i--;
		draw_line(surf, 0, st->cur, dynarr_get(st->arr, st->i));
	} else if (key == GLFW_KEY_DOWN && st->i + MAXR < st->arr->len)
	{
		i = 1;
		while (i < MAXR)
		{
			dst = surf->map + surf->sz - ((YOFF + i * FONTH) * surf->stride);
			src = surf->map + (YOFF + (MAXR - i - 1) * FONTH) * surf->stride;
			memcpy(dst, src, sz);
			i++;
		}
		st->i++;
		if ((line = dynarr_get(st->arr, st->i + MAXR - 1)))
		{
			draw_line(surf, MAXR - 1, st->cur, line);
		}
	}
	return;
}

static void glfw_key_cb(GLFWwindow* win, int key, int scancode, int action, int mods)
{
	state_t* st;
	dynstr_t* inp;

	if (action != GLFW_PRESS && action != GLFW_REPEAT)
	{
		return;
	}

	st = glfwGetWindowUserPointer(win);
	inp = st->inp;
	if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN)
	{
		handle_scroll(st, key);
	} else if (key == GLFW_KEY_F5)
	{
		load_page(st, NULL);
	} else if (key == GLFW_KEY_BACKSPACE)
	{
		if (inp->len - 1 >= 0)
		{
			/* TODO: Handle multi-byte characters properly */
			inp->ptr[--inp->len] = 0x00;
		}
		draw_url(&st->surfs[3], inp->ptr);
	} else if (key == GLFW_KEY_ENTER)
	{
		load_page(st, strdup(inp->ptr));
	}
	return;
}

static void glfw_chr_cb(GLFWwindow* win, unsigned int utf32)
{
	int rt;
	state_t* st;
	dynstr_t* inp;
	mbstate_t mbs = { 0x00 };

	st = glfwGetWindowUserPointer(win);
	inp = st->inp;
	if (dynstr_alloc(inp, inp->len + MB_CUR_MAX))
	{
		return;
	}
	rt = c32rtomb(inp->ptr + inp->len, (char32_t)utf32, &mbs);
	inp->len += rt;
	inp->ptr[inp->len] = 0x00;
	draw_url(&st->surfs[3], inp->ptr);
	return;
}

static void glfw_click_cb(GLFWwindow* win, int button, int action, int mods)
{
	double x, y;
	size_t i = 0;
	uint32_t row, col;
	state_t* st;
	dynarr_t* links;
	link_t* link;
	line_t* line;

	if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_RELEASE)
	{
		return;
	}

	st = glfwGetWindowUserPointer(win);
	glfwGetCursorPos(win, &x, &y);

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
			load_page(st, strdup(link->url));
			break;
		}
		i++;
	}
	return;
}

static void glfw_err_cb(int err, const char* str)
{
	fprintf(stderr, "GLFW Error %d: %s\n", err, str);
	return;
}

int main(int argc, char* argv[])
{
	int rt = 0;
	uint64_t i = 1;
	uint64_t tmp_sz = 0;
	thrd_t tid;
	GLFWwindow* win = NULL;

	req_t req = { 0x00 };
	res_t res = { 0x00 };
	cur_t cur = { .is_gem = false };
	dynarr_t arr;
	dynarr_t links;
	dynstr_t inp;
	surf_t surfs[] = {
		{
			.bpp = 3,
			.width = SURFW,
			.height = 0,
			.sz = 0
		},
		{
			.bpp = 3,
			.width = SURFW,
			.height = FONTH + YOFF * 2
		},
		{
			.bpp = 3,
			.width = SURFW,
			.height = MAXR * FONTH + YOFF * 2
		},
		{
			.bpp = 3,
			.width = SURFW,
			.height = FONTH + YOFF * 2
		}
	};
	state_t st = { &cur, surfs, &arr, &links, 0, &req, &res, NULL, &inp, "Loading..." };

	pipe(ps);

	surfs->stride = surfs->width * surfs->bpp;
	while (i < sizeof(surfs) / sizeof(*surfs))
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
		rt = -1;
		goto end;
	}

	i = 1;
	tmp_sz = 0;
	while (i < sizeof(surfs) / sizeof(*surfs))
	{
		surfs[i].map = surfs->map + tmp_sz;
		tmp_sz += surfs[i].sz;
		i++;
	}

	glfwSetErrorCallback(glfw_err_cb);
	if (!glfwInit())
	{
		rt = -2;
		goto end;
	}

	win = glfwCreateWindow(surfs->width, surfs->height, "Rover", NULL, NULL);
	if (!win)
	{
		rt = -3;
		goto end;
	}
	glfwSetWindowUserPointer(win, &st);
	glfwMakeContextCurrent(win);
	glfwSwapInterval(1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	draw_fill(surfs, BGCOLOR);
	glDrawPixels(surfs->width, surfs->height, GL_RGB, GL_UNSIGNED_BYTE, surfs->map);
	glfwSwapBuffers(win);

	dynarr_init(&arr, 16, line_t);
	dynarr_init(&links, 16, link_t);
	dynstr_init(&inp, MAXC);

	if ((rt -= thrd_create(&tid, worker_thread, &st)) != thrd_success)
	{
		fprintf(stderr, "Couldn't create worker thread.\n");
		goto end;
	}

	if ((rt = load_page(&st, strdup(ABOUT))))
	{
		goto end;
	}

	glfwSetKeyCallback(win, glfw_key_cb);
	glfwSetMouseButtonCallback(win, glfw_click_cb);
	glfwSetCharCallback(win, glfw_chr_cb);
	while (!glfwWindowShouldClose(win))
	{
		draw_status(&surfs[1], st.status);
		glDrawPixels(surfs->width, surfs->height, GL_RGB, GL_UNSIGNED_BYTE, surfs->map);
		glfwSwapBuffers(win);
        glfwWaitEvents();
	}
	rt = 0;

end:
	close(ps[1]);
	thrd_join(tid, NULL);
	if (win)
	{
		glfwDestroyWindow(win);
	}
	glfwTerminate();
	dynarr_free(&arr);
	dynarr_free(&links);
	dynstr_free(&inp);
	piper_free(&res, &req);
	if (st.url)
	{
		free(st.url);
	}
	if (surfs->map)
	{
		free(surfs->map);
	}
	return rt;
}
