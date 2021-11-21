#include "ares.h"

void client_close(client_t* client)
{
	close(client->timer);
	close(client->fd);
	memset(client, 0x00, sizeof(*client));
	return;
}

client_t* client_add(client_t* clients, int fd)
{
	int hash;
	int timer;
	struct itimerspec bomb = { 0x00 };

	hash = fd & (MAXCLIENTS - 1);
	while (clients[hash].stage)
	{
		hash++;
		if (hash >= MAXCLIENTS)
		{
			hash = 0;
		}
	}
	clients[hash].fd = fd;

	bomb.it_value.tv_nsec = TIMEOUT;
	/* 
	 * To be honest, you have bigger problems than a DoS attack if you're
	 * resetting the system time while internet-facing services are running.
	 */
	timer = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	timerfd_settime(timer, 0, &bomb, NULL);
	clients[hash].timer = timer;
	clients[hash].stage = 1;
	set_nonblock(fd);
	return &clients[hash];
}

/*
 * Could probably replace this with fstat, but StackOverflow
 * says a syscall would actually be less efficient.
 * [citation may be needed]
 */
client_t* client_get_or_timeout(client_t* clients, int fd)
{
	int hash;
	uint64_t tmp;

	/* First check if it's a client's socket or timer fd */
	hash = fd & (MAXCLIENTS - 1);
	while (clients[hash].stage)
	{
		if (clients[hash].fd == fd)
		{
			return &clients[hash];
		} else if (clients[hash].timer == fd)
		{
			goto timeout;
		}
		hash++;
		if (hash >= MAXCLIENTS)
		{
			hash = 0;
		}
	}

	/* Definetely not a socket, so it's probably a timer */
	hash = 0;
	while (hash < MAXCLIENTS)
	{
		if (clients[hash].timer == fd)
		{
timeout:
			read(clients[hash].timer, &tmp, sizeof(uint64_t));
			client_close(&clients[hash]);
			return NULL;
		}
		hash++;
	}

	/* We just hit some kind of obscure race condition */
	return NULL;
}
