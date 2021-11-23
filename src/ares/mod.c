#include "ares.h"

void mod_cgi_handle(client_t* client, char* path)
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

void mod_dir_handle(client_t* client, char* path)
{
	DIR* dir;
	int fd;
	struct stat st;
	struct dirent* ent;
	uint64_t sz;
	uint8_t type = CNT_GEMTEXT;
	char* index;

	dir = opendir(path);
	if (!dir)
	{
		return;
	}
	sz = htole64(UINT64_MAX);
	write(client->fd, &type, 1);
	write(client->fd, &sz, sizeof(uint64_t));

	asprintf(&index, "%s/index.gmi", path);
	if (!stat(index, &st) && (st.st_mode & S_IFMT) == S_IFREG)
	{
		fd = open(index, O_RDONLY);
		sendfile(client->fd, fd, NULL, st.st_size);
		close(fd);
		write(client->fd, "\n#", 2);
	}
	free(index);

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

void mod_file_handle(client_t* client, char* path, uint8_t type, struct stat* st)
{
	int fd;
	uint64_t sz;

	if (type == 0xff)
	{
		sz = strlen(path);
		type = CNT_RAW;
		if (sz > 4 && !strcmp(path + sz - 4, ".gmi"))
		{
			type = CNT_GEMTEXT;
		}
	}

	sz = htole64(st->st_size);
	fd = open(path, O_RDONLY);

	write(client->fd, &type, 1);
	write(client->fd, &sz, sizeof(uint64_t));
	sendfile(client->fd, fd, NULL, st->st_size);
	close(fd);
	return;
}
