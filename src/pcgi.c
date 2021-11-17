#include "pcgi.h"

int pcgi_init(pcgi_req_t* req)
{
	char* tmp;
	size_t len;

	pcgi_read(&req->sz, sizeof(req->sz));
	req->path = malloc(req->sz + 1);
	if (!req->path)
	{
		return -1;
	}
	pcgi_read(req->path, req->sz);
	req->path[req->sz] = 0x00;
	len = strlen(req->path);
	req->query = req->path + len;
	if (len < req->sz)
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

void pcgi_close(pcgi_req_t* req)
{
	fflush(stdout);
	free(req->path);
	req->path = NULL;
	return;
}
