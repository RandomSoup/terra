#include "common.h"

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

int loop_add_sigs(loop_t* loop, ...)
{
	int fd;
	int signum;
	sigset_t mask;
	va_list args;

	sigemptyset(&mask);
	va_start(args, loop);
	while ((signum = va_arg(args, int)))
	{
		sigaddset(&mask, signum);
	}
	va_end(args);
	fd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
	sigprocmask(SIG_BLOCK, &mask, NULL);
	return loop_add_fd(loop, fd);
}

void loop_destroy(loop_t* loop)
{
	loop->run = false;
	close(loop->efd);
	return;
}
