#include "rover.h"

void gem_parse(gem_t* dst, char* src)
{
	char* ws;
	char* lf;

	if (src[0] == '`' && src[1] == '`' && src[2] == '`')
	{
		dst->pre = !dst->pre;
		dst->type = EL_IGNORE;
		goto end;
	}
	if (dst->pre)
	{
		dst->type = EL_PRE;
		dst->str = src;
		goto end;
	}
	if (src[0] == '=' && src[1] == '>')
	{
		src += 2;
		dst->type = EL_LINK;
		while (isspace(*src))
		{
			src++;
		}
		if (!*src)
		{
			dst->type = EL_TEXT;
		}
		dst->url = src;
		dst->str = dst->url;
		ws = strchrnul(src, ' ');
		lf = strchrnul(src, '\n');
		if (ws < lf)
		{
			*ws = 0x00;
			do
			{
				ws++;
			} while (isspace(*ws));
			if (*ws)
			{
				dst->str = ws;
			}
		}
		*lf = 0x00;
		goto end;
	}	
	switch (*src)
	{
		case '#':
			dst->type = EL_H1;
			if (*++src == '#')
			{
				dst->type++;
				if (*++src == '#')
				{
					dst->type++;
					src++;
				}
			}
			while (isblank(*src))
			{
				src++;
			}
			dst->str = src;
		break;
		case '>':
			src++;
			dst->type = EL_QUOTE;
			while (isblank(*src))
			{
				src++;
			}
			dst->str = src;
		break;
		case '*':
			src++;
			dst->type = EL_ITEM;
			while (isblank(*src))
			{
				src++;
			}
			dst->str = src;
		break;
		default:
			dst->type = EL_TEXT;
			dst->str = src;
		break;
	}

end:
	return;
}
