#include "terra_internal.h"
#include "config.h"

int piper_build(piper_t* piper, const char* url)
{
	url = strskip(url, PIPER);
	if (!*url)
	{
		return -1;
	}
	memset(piper, 0x00, sizeof(*piper));
	piper->url = strdup(url);
	piper->host = piper->url;
	piper->uri = strdel(piper->url, '/');
	piper->port = strdel(piper->host, ':');
	if (!*piper->port)
	{
		piper->port = "60";
	}
	return 0;
}

int piper_start(piper_t* piper)
{
	int fd;
	int rt;
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
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(fd, IPPROTO_TCP, TCP_SYNCNT, &retries, sizeof(retries));

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	rt = getaddrinfo(piper->host, piper->port, &hints, &info);
	if (rt)
	{
		return -2;
	}
	memcpy(&addr, info->ai_addr, sizeof(addr));
	freeaddrinfo(info);

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)))
	{
		close(fd);
		return -3;
	}
	uri_sz = strlen(piper->uri);
	uri_lesz = htole16(uri_sz + 1);

	set_nonblock(fd);

	write(fd, &uri_lesz, sizeof(uint16_t));
	write(fd, "/", 1);
	write(fd, piper->uri, uri_sz);

	piper->fd = fd;
	return fd;
}

void piper_handle(piper_t* piper)
{
	int fd;
	int stage;
	ssize_t rt;
	void* tmp;

	fd = piper->fd;
	stage = piper->stage;
	switch (stage)
	{
		/* Read piper->hdr */
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			rt = read(fd, piper->hdr + stage, 9 - stage);
			stage += rt;
			if (rt <= 0 || piper->type == CNT_EFILE || piper->type == CNT_ESRV)
			{
				goto close_stage;
			}
		break;
		/* Process piper->sz */
		case 9:
			piper->sz = le64toh(piper->sz);
			piper->buff = malloc(piper->sz + 1);
			if (!piper->buff)
			{
				goto close_stage;
			}
			if (piper->sz >= UINT64_MAX)
			{
				stage++;
			}
			stage++;
		break;
		/* Read piper->buff (defined content length) */
		case 10:
			rt = read(fd, piper->buff + piper->off, piper->sz - piper->off);
			piper->off += rt;
			if (rt < 0)
			{
				goto close_stage;
			}
			if (piper->off == piper->sz || rt == 0)
			{
				piper->buff[piper->off] = 0x00;
				goto close_stage;
			}
		break;
		/* Read piper->buff (undefined content length) */
		case 11:
			if (ioctl(fd, SIOCINQ, &piper->rem))
			{
				goto close_stage;
			}
			tmp = realloc(piper->buff, piper->off + piper->rem + 1);
			if (!tmp)
			{
				free(piper->buff);
				goto close_stage;
			}
			piper->buff = tmp;
			rt = read(fd, piper->buff + piper->off, piper->rem);
			piper->off += rt;
			if (rt < 0)
			{
				goto close_stage;
			} else if (rt == 0)
			{
				piper->sz = piper->off - 1;
				piper->buff[piper->sz] = 0x00;
				goto close_stage;
			}
		break;
		default:
close_stage:
			close(fd);
			stage = 0;
			fd = 0;
		break;
	}
	piper->fd = fd;
	piper->stage = stage;
	return;
}

void piper_free(piper_t* piper)
{
	if (piper->fd > 0)
	{
		close(piper->fd);
	}
	if (piper->buff)
	{
		free(piper->buff);
	}
	if (piper->url)
	{
		free(piper->url);
	}
	memset(piper, 0x00, sizeof(*piper));
	piper->type = CNT_GEMTEXT;
	return;
}
