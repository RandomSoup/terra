#include "rover.h"
#include "config.h"

int line_next(line_t* line, cur_t* cur)
{
	int rt;
	char* ptr;
	char32_t wchr;
	uint8_t i = 0;
	ptrdiff_t diff = 0;
	mbstate_t mbs = { 0x00 };
	gem_t* gem;

	ptr = cur->str + cur->off;
	if (!mbrtoc32(NULL, ptr, MB_CUR_MAX, &mbs))
	{
		return -1;
	}
	if (cur->is_gem)
	{
		gem = &cur->gem;
		if (cur->type == EL_IGNORE)
		{
			gem_parse(gem, ptr);
			cur->type = gem->type;
		}
		if (gem->type == EL_LINK)
		{
			diff = gem->str - ptr;
		}
		ptr += diff;
		cur->off += diff;
		line->type = cur->type;
	} else
	{
		line->type = EL_TEXT;
	}
	rt = mbrtoc32(&wchr, ptr, MB_CUR_MAX, &mbs);
	if (cur->type != EL_IGNORE && wchr == ' ')
	{
		ptr++;
		cur->off++;
	}
	while (i < MAXC)
	{
		rt = mbrtoc32(&wchr, ptr, MB_CUR_MAX, &mbs);
		if (rt < 0)
		{
			ptr += MB_CUR_MAX;
		} else
		{
			if (wchr == '\t')
			{
				*ptr = ' ';
			}
			ptr += rt;
			if (!rt || wchr == '\n')
			{
				cur->type = EL_IGNORE;
				break;
			}
		}
		i++;
	}
	line->sz = i;
	line->off = cur->off;
	if (cur->type == EL_IGNORE)
	{
		i++;
	}
	cur->off += i;
	return 0;
}
