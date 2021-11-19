#include "rover.h"

int _dynarr_init(dynarr_t* arr, size_t sz, size_t type)
{
	arr->ptr = malloc(sz * type);
	if (!arr->ptr)
	{
		return -1;
	}
	arr->sz = sz;
	arr->len = 0;
	arr->type = type;
	return 0;
}

int dynarr_add(dynarr_t* arr, void* ptr)
{
	void* tmp;

	if (arr->sz <= arr->len * 1.125)
	{
		arr->sz *= 1.5;
		tmp = realloc(arr->ptr, arr->sz * arr->type);
		if (!tmp)
		{
			return -1;
		}
		arr->ptr = tmp;
	}
	memcpy(arr->ptr + arr->len * arr->type, ptr, arr->type);
	arr->len++;
	return 0;
}

void* dynarr_get(dynarr_t* arr, size_t i)
{
	if ((ssize_t)i >= arr->len)
	{
		return NULL;
	}
	return arr->ptr + i * arr->type;
}

void dynarr_free(dynarr_t* arr)
{
	if (arr->ptr)
	{
		free(arr->ptr);
	}
	memset(arr, 0x00, sizeof(*arr));
	return;
}

void dynarr_reset(dynarr_t* arr)
{
	arr->len = 0;
	return;
}
