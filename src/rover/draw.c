/* TODO: Maybe make this self-contained, somehow? */

#include "rover.h"
#define DEF_PALETTE
#include "config.h"
#include "font.h"

int draw_chr(surf_t* surf, char32_t chr, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg)
{
	uint8_t cur;
	uint32_t j = 0;
	uint32_t i = UINT32_MAX;
	const uint8_t* bmp;

	if (chr < FONTCC && chr >= 0x20 && font_idx[chr - 0x20] == chr)
	{
		i = chr - 0x20;
	} else
	{
		for (j = 0; j < FONTCC; j++)
		{
			if (font_idx[j] == chr)
			{
				i = j;
				break;
			}
		}
	}
	if (i == UINT32_MAX)
	{
		fprintf(stderr, "Error: Character U+%04x not found in font.\n", chr);
		return -1;
	}
	bmp = font_bmp + FONTH * i;
	for (i = 0; i < FONTH; i++)
	{
		cur = bmp[i];
		for (j = FONTW; j; j--)
		{
			if (cur & 1 << (j - 1))
			{
				draw_setpx(surf, x - j, y + i, fg);
			} else
			{
				draw_setpx(surf, x - j, y + i, bg);
			}
		}
	}
	return 0;
}

void draw_fill(surf_t* surf, uint32_t color)
{
	uint64_t sz;
	uint32_t* px;

	px = (uint32_t*)surf->map;
	sz = surf->height * surf->width;
	for (uint64_t i = 0; i < sz; i++, px++)
	{
		*px = color;
	}
	return;
}

void draw_str(surf_t* surf, char* str, uint64_t sz, int row, uint32_t color)
{
	int rt = 0;
	char32_t wchr;
	mbstate_t mbs = { 0x00 };

	for (uint64_t i = 0; i < MAXC; i++)
	{
		if (i < sz)
		{
			rt = mbrtoc32(&wchr, str, MB_CUR_MAX, &mbs);
			if (rt < 0)
			{
				str += MB_CUR_MAX;
			} else
			{
				str += rt;
			}
		} else
		{
			wchr = L' ';
		}
		draw_chr(surf, wchr, XOFF + FONTW * (i + 1), YOFF + FONTH * row, color, BGCOLOR);
	}
	return;
}

void draw_line(surf_t* surf, line_t* line, cur_t* cur, int row)
{
	return draw_str(surf, cur->str + line->off, line->sz, row, palette[line->type]);
}

int draw_url(surf_t* surf, char* url)
{
	uint32_t i = 0;

	draw_str(surf, url, strlen(url), 0, palette[EL_TEXT]);
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

	draw_str(surf, (char*)status, strlen(status), 0, palette[EL_TEXT]);
	while (i < surf->width)
	{
		draw_setpx(surf, i, 0, palette[EL_H1]);
		i++;
	}
	return 0;
}
