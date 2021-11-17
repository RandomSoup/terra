#include "rover.h"
#define DEF_PALETTE
#include "config.h"
#include "font.h"

always_inline void draw_setpx(surf_t* surf, uint32_t x, uint32_t y, uint32_t color)
{
	uint32_t* px;

	px = (uint32_t*)(surf->map + x * surf->bpp + y * surf->stride);
	*px = color;
	return;
}

int draw_chr(surf_t* surf, char32_t chr, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg)
{
	uint8_t cur;
	uint32_t i = (uint32_t)-1;
	uint32_t j = 0;
	const uint8_t* bmp;

	if (chr == 0x00)
	{
		return 0;
	}

	if (chr < FONTCC && chr >= 0x20 && font_idx[chr - 0x20] == chr)
	{
		i = chr - 0x20;
	} else
	{
		while (j < FONTCC)
		{
			if (font_idx[j] == chr)
			{
				i = j;
				break;
			}
			j++;
		}
	}
	if (i == (uint32_t)-1)
	{
		fprintf(stderr, "Error: Character U+%04x not found in font.\n", chr);
		return -1;
	}
	bmp = font_bmp + FONTH * i;
	i = 0;
	while (i < FONTH)
	{
		j = FONTW;
		cur = bmp[i];
		while (j)
		{
			if (cur & 1 << (j - 1))
			{
				draw_setpx(surf, x - j, y + i, fg);
			} else
			{
				draw_setpx(surf, x - j, y + i, bg);
			}
			j--;
		}
		i++;
	}
	return 0;
}

void draw_fill(surf_t* surf, uint32_t color)
{
	uint64_t sz;
	uint64_t i = 0;
	uint32_t* px;

	px = (uint32_t*)surf->map;
	sz = surf->height * surf->width;
	while (i < sz)
	{
		*px = color;
		px++;
		i++;
	}
	return;
}

int draw_str(surf_t* surf, int row, char* str, uint64_t sz, uint32_t color)
{
	int rt = 0;
	char32_t wchr;
	uint64_t i = 0;
	mbstate_t mbs = { 0x00 };

	while (i < MAXC)
	{
		rt = mbrtoc32(&wchr, str, MB_CUR_MAX, &mbs);
		if (rt < 0)
		{
			str += MB_CUR_MAX;
		} else
		{
			str += rt;
		}
		draw_chr(surf, i < sz ? wchr : L' ', XOFF + FONTW * (i + 1), YOFF + FONTH * row, color, BGCOLOR);
		i++;
	}
	return 0;
}

int draw_line(surf_t* surf, int row, cur_t* cur, line_t* line)
{
	return draw_str(surf, row, cur->str + line->off, line->sz, palette[line->type]);
}

int draw_url(surf_t* surf, char* url)
{
	uint32_t i = 0;

	draw_str(surf, 0, url, strlen(url), palette[EL_TEXT]);
	while (i < surf->width)
	{
		draw_setpx(surf, i, YOFF * 2 + FONTH - 1, palette[EL_H1]);
		i++;
	}
	return 0;
}

int draw_status(surf_t* surf, const char* status)
{
	uint32_t i = 0;

	draw_str(surf, 0, (char*)status, strlen(status), palette[EL_TEXT]);
	while (i < surf->width)
	{
		draw_setpx(surf, i, 0, palette[EL_H1]);
		i++;
	}
	return 0;
}
