#include "ares.h"

#include "cwalk.h"

static char dir[PATH_MAX + 1];
static size_t dir_len = 0;

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

always_inline void handle_cgi(client_t* client, char* path)
{
	int pi[2];
	pid_t pid;
	char* argv[2] = { NULL, NULL };

	pipe(pi);

	write(pi[1], &client->sz, sizeof(client->sz));
	write(pi[1], client->uri, client->sz);

	pid = fork();
	if (!pid)
	{
		close(pi[1]);
		dup2(pi[0], STDIN_FILENO);
		dup2(client->fd, STDOUT_FILENO);
		argv[0] = path;
		execvp(path, argv);
	}
	close(pi[0]);
	return;
}

always_inline void handle_req(client_t* client)
{
	int fd;
	char* query;
	uint64_t sz = 0;
	uint8_t type = 0x22;
	struct stat st;
	char full[UINT16_MAX + 1];

	client->uri[client->sz] = 0x00;
	printf("Requested %s\n", client->uri);
	query = strchrnul(client->uri, '?');
	*query = 0x00;

	sz = cwk_path_get_absolute("/", client->uri, full + dir_len, UINT16_MAX - dir_len - 1);
	memcpy(full, dir, dir_len);
	full[dir_len] = '/';
	full[dir_len + sz] = 0x00;
	if (stat(full, &st))
	{
		goto err;
	}
	type = CNT_UTF8;
	if (sz > 4)
	{
		if (!strcmp(full + dir_len + sz - 4, ".gmi"))
		{
			type = CNT_GEMTEXT;
		} else if (!strcmp(full + dir_len + sz - 4, ".fwd"))
		{
			type = CNT_REDIR;
		} else if (!strcmp(full + dir_len + sz - 4, ".cgi"))
		{
			handle_cgi(client, full);
			goto end;
		}
	}
	sz = htole64(st.st_size);
	fd = open(full, O_RDONLY);

	write(client->fd, &type, 1);
	write(client->fd, &sz, sizeof(uint64_t));
	sendfile(client->fd, fd, NULL, st.st_size);
	close(fd);
	goto end;

err:
	write(client->fd, &type, 1);
	write(client->fd, &sz, sizeof(uint64_t));

end:
	return;
}

always_inline int handle_client(client_t* client)
{
	int tmp;

	switch (client->stage)
	{
		/* Receive client->szb[0] and client->szb[1] (maybe) */
		case 1:
			tmp = read(client->fd, client->szb, sizeof(uint16_t));
			client->stage += tmp;
			if (tmp == sizeof(uint16_t))
			{
				client->sz = le16toh(client->sz);
				client->uri = malloc(client->sz + 1);
				if (!client->uri)
				{
					return -1;
				}
			}
		break;
		/* Receive client->szb[1] */
		case 2:
			tmp = read(client->fd, client->szb + 1, sizeof(uint8_t));
			client->stage += tmp;
			if (tmp == sizeof(uint16_t))
			{
				client->sz = le16toh(client->sz);
				client->uri = malloc(client->sz + 1);
				if (!client->uri)
				{
					return -1;
				}
			}
		break;
		/* Receive client->uri */
		case 3:
			while (true)
			{
				tmp = read(client->fd, client->uri + client->csz, client->sz - client->csz);
				if (tmp >= 0)
				{
					client->csz += tmp;
					if (client->csz == client->sz)
					{
						handle_req(client);
						return 1;
					}
				} else
				{
					if (errno != EWOULDBLOCK)
					{
						return -2;
					}
					break;
				}
			}
		break;
		default:
		break;
	}
	return 0;
}

int main(int argc, char* argv[])
{
	int tmp;
	int server;
	int reuse = 1;
	int clientc = 0;
	short port = 60;
	unsigned int addrlen;
	struct signalfd_siginfo si;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	loop_t loop;
	client_t* client;
	client_t clients[MAXCLIENTS];

	if (argc < 3)
	{
		fprintf(stderr, "Usage: %s <port> <dir>\n", argv[0]);
		return -1;
	}
	port = atoi(argv[1]);
	realpath(argv[2], dir);
	dir_len = strlen(dir);
	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server < 0)
	{
		goto err;
	}
	setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuse, sizeof(int));

	memset(&server_addr, 0x00, ADDRLEN);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	if (bind(server, (struct sockaddr*)&server_addr, ADDRLEN) < 0) 
	{
		goto err;
	}
	if (listen(server, 5) < 0)
	{
		goto err;
	}

	addrlen = ADDRLEN;
	loop_init(&loop);
	printf("Ares " VERSION " listening on port %d...\n", port);

	loop.xfd = loop_add_sigs(&loop, SIGINT, SIGTERM, SIGCHLD, NULL);
	loop_add_fd(&loop, server);
	loop_run(loop, {}, {
		if (loop_fd == loop.xfd)
		{
			read(loop_fd, &si, sizeof(si));
			if (si.ssi_signo == SIGCHLD)
			{
				wait(NULL);
			} else
			{
				loop.run = false;
			}
		} else if (loop_fd == server)
		{
			tmp = accept(server, (struct sockaddr*)&client_addr, &addrlen);
			if (tmp < 0)
			{
				continue;
			}
			if (clientc >= MAXCLIENTS)
			{
				close(tmp);
			}
			clients_set(clients, tmp);
			loop_add_fd(&loop, tmp);
			clientc++;
		} else
		{
			client = clients_get(clients, loop_fd);
			if (client && handle_client(client))
			{
				close(loop_fd);
				clients_del(clients, loop_fd);
			}
		}
	});
	loop_destroy(&loop);
	close(server);
	return 0;

err:
	perror("Error");
	if (server >= 0)
	{
		close(server);
	}
	return -1;
}
