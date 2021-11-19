#include "ares.h"

int clients_set(client_t* clients, int fd)
{
	int hash;
	int flags;

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
	clients[hash].stage = 1;
	flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	return hash;
}

client_t* clients_get(client_t* clients, int fd)
{
	int hash;

	hash = fd & (MAXCLIENTS - 1);
	while (clients[hash].stage)
	{
		if (clients[hash].fd == fd)
		{
			return &clients[hash];
		}
		hash++;
		if (hash >= MAXCLIENTS)
		{
			hash = 0;
		}
	}
	return NULL;
}

void clients_del(client_t* clients, int fd)
{
	int hash;

	hash = fd & (MAXCLIENTS - 1);
	while (clients[hash].stage)
	{
		if (clients[hash].fd == fd)
		{
			if (clients[hash].uri)
			{
				free(clients[hash].uri);
			}
			memset(&clients[hash], 0x00, sizeof(clients[hash]));
			break;
		}
		hash++;
		if (hash >= MAXCLIENTS)
		{
			hash = 0;
		}
	}
	return;
}
