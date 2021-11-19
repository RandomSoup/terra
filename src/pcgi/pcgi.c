#include "pcgi.h"

int pcgi_init(pcgi_req_t* req)
{
	size_t sz;

	pcgi_read(&req->sz, sizeof(req->sz));
	req->path = malloc(req->sz + 1);
	if (!req->path)
	{
		return -1;
	}
	pcgi_read(req->path, req->sz);
	req->path[req->sz] = 0x00;
	sz = strlen(req->path);
	req->query = req->path + sz;
	if (sz < req->sz)
	{
		req->query++;
	}
	return 0;
}

pcgi_req_t* pcgi_new()
{
	pcgi_req_t* req;

	req = malloc(sizeof(*req));
	memset(req, 0x00, sizeof(*req));
	if (req)
	{
		pcgi_init(req);
		pcgi_set_env(req);
	}
	return req;
}

void pcgi_hdr(uint8_t type)
{
	uint64_t max;

	max = htole64(UINT64_MAX);
	pcgi_write(&type, sizeof(type));
	pcgi_write(&max, sizeof(max));
	return;
}

int pcgi_set_env(pcgi_req_t* req)
{
	char* tmp;
	char* arg;
	char* start;
	char* end;
	char* val;
	char* query;
	size_t sz;
	size_t query_sz;

	setenv(PREFIX "path", req->path, 1);

	query = start = end = req->query;
	arg = malloc(PREFIX_SZ + 32);
	memcpy(arg, PREFIX, PREFIX_SZ);
	query_sz = strlen(query);
	while (TRUE)
	{
		end = strchrnul(end, '&');
		*end = 0x00;
		sz = end - start;
		tmp = realloc(arg, sz + PREFIX_SZ);
		if (!tmp)
		{
			free(arg);
			return -1;
		}
		arg = tmp;
		memcpy(arg + PREFIX_SZ, start, sz + 1);
		val = strdel(arg, '=');
		setenv(arg, val, 0);
		if (query + query_sz <= end)
		{
			break;
		}
		start = ++end;
	}
	free(arg);
	return 0;
}

void pcgi_close(pcgi_req_t* req)
{
	fflush(stdout);
	free(req->path);
	req->path = NULL;
	return;
}
