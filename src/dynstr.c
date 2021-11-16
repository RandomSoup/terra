#include "rover.h"

int dynstr_init(dynstr_t* str, size_t sz)
{
	str->ptr = malloc(sz + 1);
	if (!str->ptr)
	{
		return -1;
	}
	str->sz = sz + 1;
	str->len = 0;
	*str->ptr = 0x00;
	return 0;
}

int dynstr_alloc(dynstr_t* str, ssize_t sz)
{
	char* tmp;

	if (str->sz <= sz + 1)
	{
		str->sz = sz + 1;
		tmp = realloc(str->ptr, str->sz);
		if (!tmp)
		{
			return -1;
		}
		str->ptr = tmp;
	}
	return 0;
}

int dynstr_set(dynstr_t* str, char* ptr)
{
	char* tmp;
	ssize_t sz;

	sz = strlen(ptr);
	if (str->sz <= sz + 1)
	{
		str->sz = sz + 1;
		tmp = realloc(str->ptr, str->sz);
		if (!tmp)
		{
			return -1;
		}
		str->ptr = tmp;
	}
	memcpy(str->ptr, ptr, sz + 1);
	str->len = sz;
	return 0;
}

void dynstr_free(dynstr_t* str)
{
	if (str->ptr)
	{
		free(str->ptr);
	}
	memset(str, 0x00, sizeof(*str));
	return;
}
