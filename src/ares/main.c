#include "ares.h"

always_inline void handle_dir(client_t* client, char* path)
{
	DIR* dir;
	struct dirent* ent;
	uint64_t sz;
	uint8_t type = CNT_GEMTEXT;

	dir = opendir(path);
	if (!dir)
	{
		return;
	}
	sz = htole64(UINT64_MAX);
	write(client->fd, &type, 1);
	write(client->fd, &sz, sizeof(uint64_t));
	write(client->fd, DIR_HDR, DIR_HDR_SZ);
	while ((ent = readdir(dir)))
	{
		if (*ent->d_name == '.')
		{
			continue;
		}
		write(client->fd, "=> ", LINK_SZ);
		write(client->fd, ent->d_name, strlen(ent->d_name));
		write(client->fd, "\n", 1);
	}
	write(client->fd, DIR_FTR, DIR_FTR_SZ);
	closedir(dir);
	close(client->fd);
	return;
}

always_inline void handle_cgi(client_t* client, char* path)
{
	int ps[2];
	pid_t pid;

	pipe(ps);

	write(ps[1], &client->sz, sizeof(client->sz));
	write(ps[1], client->uri, client->sz);

	pid = fork();
	if (!pid)
	{
		close(ps[1]);
		dup2(ps[0], STDIN_FILENO);
		dup2(client->fd, STDOUT_FILENO);
		execl(path, path, NULL);
	}
	close(ps[0]);
	return;
}

always_inline void handle_req(client_t* client)
{
	int fd;
	uint64_t sz = 0;
	uint8_t type = 0x22;
	struct stat st;
	char* cwd = NULL;
	char* full = NULL;
	char tmp[UINT16_MAX + 8];

	client->uri[client->sz] = 0x00;
	printf("Requested %s\n", client->uri);
	strdel(client->uri, '?');

	cwd = getcwd(NULL, 0);
	sprintf(tmp, "./%s", client->uri);
	full = realpath(tmp, NULL);
	if (!cwd || !full || !strsubs(full, cwd))
	{
		goto err;
	}

	if (stat(full, &st))
	{
		goto err;
	}
	if ((st.st_mode & S_IFMT) == S_IFDIR)
	{
		handle_dir(client, full);
		goto end;
	}
	type = CNT_UTF8;
	sz = strlen(full);
	if (sz > 4)
	{
		if (!strcmp(full + sz - 4, ".gmi"))
		{
			type = CNT_GEMTEXT;
		} else if (!strcmp(full + sz - 4, ".fwd"))
		{
			type = CNT_REDIR;
		} else if (!strcmp(full + sz - 4, ".cgi"))
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
	if (full)
	{
		free(full);
	}
	if (cwd)
	{
		free(cwd);
	}
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
			tmp = read(client->fd, client->uri + client->csz, client->sz - client->csz);
			if (tmp >= 0)
			{
				client->csz += tmp;
				if (client->csz == client->sz)
				{
					handle_req(client);
					return 1;
				}
			} else if (errno != EWOULDBLOCK)
			{
				return -2;
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
	int sig;
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

	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		return -1;
	}
	port = atoi(argv[1]);
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

	sig = loop_add_sigs(&loop, SIGINT, SIGTERM, SIGCHLD, NULL);
	loop_add_fd(&loop, server);
	loop_run(loop, {}, {
		if (loop_fd == sig)
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
			client = client_add(clients, tmp);
			loop_add_fd(&loop, tmp);
			loop_add_fd(&loop, client->timer);
			clientc++;
		} else
		{
			client = client_get_or_timeout(clients, loop_fd);
			if (!client)
			{
				clientc--;
			} else if (handle_client(client))
			{
				client_close(client);
				clientc--;
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
