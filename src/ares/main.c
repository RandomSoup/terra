#include "ares.h"

tr_always_inline void handle_req(client_t* client, dynarr_t* arr)
{
	uint64_t sz = 0;
	uint8_t type = 0x22;
	struct stat st;
	char* cwd = NULL;
	char* full = NULL;
	char tmp[UINT16_MAX + 8] = "./";
	cfg_t cfg;

	cwd = getcwd(NULL, 0);

	if (client->uri)
	{
		client->uri[client->sz] = 0x00;
		printf("Requested %s\n", client->uri);
		strdel(client->uri, '?');
		sprintf(tmp, "./%s", client->uri);
	} else
	{
		printf("Void request\n");
	}

	full = realpath(tmp, NULL);
	if (!cwd || !full || !strsubs(full, cwd) || stat(full, &st))
	{
		goto err;
	}
	cfg_get(&cfg, arr->ptr, arr->len, client->uri);

	/* TODO: Implement caching */
	sz = strlen(full);
	if (cfg.dir && ((st.st_mode & S_IFMT) == S_IFDIR))
	{
		mod_dir_handle(client, full);
	} else if (cfg.cgi)
	{
		mod_cgi_handle(client, full);
	} else
	{
		mod_file_handle(client, full, cfg.type, &st);
	}
	goto end;

err:
	write(client->fd, &type, 1);
	write(client->fd, &sz, sizeof(uint64_t));

end:
	if (client->uri)
	{
		free(client->uri);
	}
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

tr_always_inline int handle_client(client_t* client, dynarr_t* cfg)
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
				if (!client->sz)
				{
					goto handle;
				}
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
handle:
					handle_req(client, cfg);
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
	int reuse = 1;
	int server = 0;
	int clientc = 0;
	short port = 60;
	unsigned int addrlen;
	struct signalfd_siginfo si;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	loop_t loop;
	client_t* client;
	client_t clients[MAXCLIENTS] = { 0x00 };
	dynarr_t arr;
	cfg_t* cfg;

	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <port> [config_path]\n", argv[0]);
		return -1;
	}
	port = atoi(argv[1]);
	if (cfg_load(&arr, argc > 2 ? argv[2] : CFG_PATH))
	{
		goto err;
	}
	cfg = ((cfg_t*)arr.ptr);
	/* TODO: Implement multiple roots */
	if (cfg->root)
	{
		chdir(cfg->root);
	}

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
			} else if (handle_client(client, &arr))
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
