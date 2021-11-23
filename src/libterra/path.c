#include "terra_internal.h"

static char* piper_path_del(char* path, size_t path_sz, char* part, size_t part_sz)
{
	char* last;
	size_t tmp_sz;

	last = part - 1;
	while (*last != '/' && last >= path)
	{
		last--;
	}
	if (part + part_sz <= path + path_sz)
	{
		tmp_sz = strlen(part + part_sz);
		memmove(++last, part + part_sz, tmp_sz);
		last[tmp_sz] = 0x00;
	}
	return path;
}

static char* piper_path_normalize(char* path)
{
	size_t i;
	size_t sz;
	char* next;
	bool do_parent;

	next = path;
	do_parent = true;
	while (*next)
	{
		if (*next == '/')
		{
			if (next[1] == '/')
			{
				i = 1;
				while (next[i] == '/')
				{
					i++;
				}
				sz = strlen(next + i);
				memcpy(next, next + i, sz);
				next[sz] = 0x00;
			} else if (next[1] == '.' && next[2] == '/')
			{
				sz = strlen(next + 2);
				memcpy(next, next + 2, sz);
				next[sz] = 0x00;
			} else if (do_parent && !strncmp(next, "/../", 4))
			{
				piper_path_del(path, strlen(path), next, PARENT_SZ);
				if (!strncmp(path, PARENT, PARENT_SZ))
				{
					do_parent = false;
				}
			}
		}
		next++;
	}
	return path;
}

static char* piper_path_join(char* dst, char* base, size_t base_sz, char* path, size_t path_sz, size_t max_sz)
{
	if (*base != '/')
	{
		*dst = '/';
		dst++;
	}
	memcpy(dst, base, base_sz);
	if (dst[base_sz - 1] != '/')
	{
		dst[base_sz] = '/';
		base_sz++;
	}
	memcpy(dst + base_sz, path, path_sz);
	dst[base_sz + path_sz] = 0x00;
	return piper_path_normalize(--dst);
}

char* piper_path_resolve(piper_t* piper, char* target, bool redir)
{
	char* rt;
	size_t total_sz;
	size_t host_sz;
	size_t port_sz;
	size_t hp_sz;
	size_t old_sz;
	size_t target_sz;

	if (strsubs(target, PIPER))
	{
		rt = strdup(target);
	} else if (*target == '/')
	{
		asprintf(&rt, "%s:%s%s", piper->host, piper->port, target);
	} else
	{
		host_sz = strlen(piper->host);
		port_sz = strlen(piper->port);
		hp_sz = host_sz + port_sz + 1;
		piper_path_normalize(piper->uri);
		if (redir)
		{
			old_sz = strlen(piper->uri);
			piper_path_del(piper->uri, old_sz, piper->uri + old_sz, 0);
		}
		piper_path_normalize(target);
		old_sz = strlen(piper->uri);
		target_sz = strlen(target);
		total_sz = hp_sz + old_sz + target_sz + 4;
		rt = malloc(total_sz);
		if (!rt)
		{
			return NULL;
		}
		memcpy(rt, piper->host, host_sz);
		rt[host_sz] = ':';
		memcpy(rt + host_sz + 1, piper->port, port_sz);
		piper_path_join(rt + hp_sz, piper->uri, old_sz, target, target_sz, total_sz - hp_sz);
	}
	return rt;
}
