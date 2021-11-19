#ifndef PCGI_H
#define PCGI_H

#include "common.h"

#define PREFIX "PCGI_"
#define PREFIX_SZ (sizeof(PREFIX) - 1)

typedef struct pcgi_req_t
{
	char* path;
	char* query;
	uint16_t sz;
} pcgi_req_t;

pcgi_req_t* pcgi_new();
int pcgi_init(pcgi_req_t* req);
void pcgi_hdr(uint8_t type);
void pcgi_close(pcgi_req_t* req);
int pcgi_set_env(pcgi_req_t* req);

#define pcgi_read(buff, sz) read(STDIN_FILENO, (buff), (sz))
#define pcgi_write(buff, sz) write(STDOUT_FILENO, (buff), (sz))

#ifdef PCGI_DSL

#define el(f) for (char* t = NULL; !t; printf(f " %s\n", t)) t =

#define h1 el("#")
#define h2 el("##")
#define h3 el("###")
#define ul el("*")
#define quot el(">")
#define link(u) for (char* t = NULL; !t; printf("=> %s %s\n", u, t)) t =
#define pre for (bool t = (puts("```"), true); t; t = (puts("```"), false))
#define type for (uint8_t t = 0xff; t == 0xff; pcgi_hdr(t)) t = (uint8_t)
#define p for (char* t = NULL; !t; printf("%s\n", t)) t =

#define pcgi for (pcgi_req_t* req = pcgi_new(); req->path; pcgi_close(req))

#endif /* PCGI_DSL */

#endif /* !PCGI_H */
