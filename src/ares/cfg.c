#include "ares.h"

static int cfg_cmp(const void* p0, const void* p1)
{
	const cfg_t* c0 = p0;
	const cfg_t* c1 = p1;

	return strcmp(c0->glob, c1->glob);
}

tr_always_inline void cfg_set(cfg_t* cfg, char* key, char* val)
{
#define PROP(type, name) if (!strcmp(STR(name), key)) \
{ \
	cfg->name = _Generic((cfg->name), \
		char: *val, \
		uint8_t: strtol(val, NULL, 0), \
		uint16_t: strtol(val, NULL, 0), \
		uint32_t: strtol(val, NULL, 0), \
		uint64_t: strtoll(val, NULL, 0), \
		bool: !strcmp(val, "true"), \
		char*: strdup(val), \
		default: (void*)val \
	); \
}
#include "ares_props.h"
#undef PROP
	return;
}

tr_always_inline void cfg_cpy(cfg_t* dst, cfg_t* src)
{
#define PROP(type, name) if (src->name) \
{ \
	dst->name = src->name; \
}
#include "ares_props.h"
#undef PROP
	return;
}

int cfg_load(dynarr_t* arr, char* path)
{
	FILE* fd;
	size_t sz = 0;
	char* line = NULL;
	char* tmp = NULL;
	char* start = NULL;
	char* key = NULL;
	char* val = NULL;
	cfg_t cfg = { 0x00 };

	fd = fopen(path, "r");
	if (!fd)
	{
		return -1;
	}
	dynarr_init(arr, 8, cfg_t);
	cfg.glob = strdup("*");
	cfg.dir = true;
	cfg.type = 0xff;
	while (getline(&line, &sz, fd) > 0)
	{
		strdel(line, '\n');
		if (!*line || *line == '#')
		{
			continue;
		}
		tmp = line;
		if (*line == '[')
		{
			while (isblank(*tmp))
			{
				tmp++;
			}
			if (!*tmp || *tmp == ']')
			{
				continue;
			}
			tmp++;
			start = tmp;
			while (*tmp && !isblank(*tmp) && *tmp != ']')
			{
				tmp++;
			}
			while (isblank(*tmp))
			{
				tmp++;
			}
			if (*tmp != ']')
			{
				continue;
			}
			*tmp = 0x00;
			dynarr_add(arr, &cfg);
			memset(&cfg, 0x00, sizeof(cfg));
			cfg.dir = true;
			cfg.type = 0xff;
			cfg.glob = strdup(start);
		} else
		{
			key = line;
			while (*tmp && !isblank(*tmp) && *tmp != '=')
			{
				tmp++;
			}
			if (!*tmp)
			{
				continue;
			}
			*tmp = 0x00;
			tmp++;
			while (isblank(*tmp))
			{
				tmp++;
			}
			if (*tmp != '=')
			{
				continue;
			}
			tmp++;
			while (isblank(*tmp))
			{
				tmp++;
			}
			if (!*tmp)
			{
				continue;
			}
			val = tmp;
			cfg_set(&cfg, key, val);
		}
	}
	dynarr_add(arr, &cfg);
	qsort(arr->ptr, arr->len, sizeof(cfg), cfg_cmp);

	free(line);
	fclose(fd);
	return 0;
}

void cfg_get(cfg_t* cfg, cfg_t* cfgs, size_t sz, char* uri)
{
	memset(cfg, 0x00, sizeof(*cfg));

	for (size_t i = 0; i < sz; i++)
	{
		if (fnmatch(cfgs[i].glob, uri, 0))
		{
			continue;
		}
		cfg_cpy(cfg, &cfgs[i]);
	}
	return;
}

void cfg_free(dynarr_t* arr)
{
	cfg_t* ptr;

	for (ssize_t i = 0; i < arr->len; i++)
	{
		ptr = ((cfg_t*)arr->ptr) + i;
		free(ptr->glob);
		if (ptr->root)
		{
			free(ptr->root);
		}
	}
	dynarr_free(arr);
}
