#include "www.h"

const char* dir;

int loop_init(loop_t* loop)
{
	loop->efd = epoll_create1(0);
	loop->run = false;
	if (loop->efd < 0)
	{
		return -1;
	}
	return 0;
}

int loop_add_fd(loop_t* loop, int fd)
{
	struct epoll_event ev;

	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(loop->efd, EPOLL_CTL_ADD, fd, &ev) == -1)
	{
		return -1;
	}
	return fd;
}

void loop_destroy(loop_t* loop)
{
	loop->run = false;
	close(loop->efd);
	return;
}
