#include "common.h"

size_t strsubs(const char* str, const char* sub)
{
	size_t len;

	len = strlen(sub);
	if (!memcmp(str, sub, len))
	{
		return len;
	}
	return 0;
}

const char* strskip(const char* str, const char* sub)
{
	return str + strsubs(str, sub);
}

char* strdel(char* str, char chr)
{
	char* tmp;

	tmp = strchrnul(str, chr);
	if (*tmp)
	{
		*tmp = 0x00;
		tmp++;
	}
	return tmp;
}

int set_nonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
