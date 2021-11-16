/* Hic sunt dracones */

#include "rover.h"
#include "config.h"

int piper_build(req_t* req, res_t* res, const char* url)
{
	char* slash;
	char* colon;

	if (!strncmp(url, PIPER, sizeof(PIPER) - 1))
	{
		url += sizeof(PIPER) - 1;
	}
	if (!*url)
	{
		return -1;
	}
	req->url = strdup(url);
	req->host = req->url;
	slash = strchrnul(req->url, '/');
	*slash = 0x00;
	req->uri = slash + 1;
	colon = strchr(req->host, ':');
	if (colon)
	{
		*colon = 0x00;
		req->port = colon + 1;
	} else
	{
		req->port = "60";
	}
	return 0;
}

int piper_start(res_t* res, req_t* req)
{
	int fd;
	int rt;
	int flags;
	int retries;
	uint16_t uri_sz;
	uint16_t uri_lesz;
	struct sockaddr_in addr;
	struct addrinfo hints;
	struct addrinfo* info;
	struct timeval tv;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		return -1;
	}

	retries = RETRIES;
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const void*)&tv, sizeof(tv));
	setsockopt(fd, IPPROTO_TCP, TCP_SYNCNT, &retries, sizeof(retries));

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	rt = getaddrinfo(req->host, req->port, &hints, &info);
	if (rt)
	{
		return -2;
	}
	memcpy(&addr, ((struct sockaddr_in*)info->ai_addr), sizeof(addr));
	freeaddrinfo(info);

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)))
	{
		close(fd);
		return -3;
	}
	uri_sz = strlen(req->uri);
	uri_lesz = htole16(uri_sz + 1);

	flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	write(fd, &uri_lesz, sizeof(uint16_t));
	write(fd, "/", 1);
	write(fd, req->uri, uri_sz);
	return fd;
}

void piper_free(res_t* res, req_t* req)
{
	if (res->buff)
	{
		free(res->buff);
	}
	if (req->url)
	{
		free(req->url);
	}
	memset(req, 0x00, sizeof(*req));
	memset(res, 0x00, sizeof(*res));
	res->type = CNT_GEMTEXT;
	return;
}
