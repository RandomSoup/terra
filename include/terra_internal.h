#ifndef TERRA_INTERNAL_H
#define TERRA_INTERNAL_H

#define _GNU_SOURCE
#include <stdarg.h>

#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <endian.h>
#include <netdb.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/signalfd.h>
#include <sys/ioctl.h>
#include <sys/timerfd.h>

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/sockios.h>

#include "terra.h"

#define PIPER "piper://"
#define PARENT "/../"
#define PARENT_SZ (sizeof(PARENT) - 1)

#define PREFIX "PCGI_"
#define PREFIX_SZ (sizeof(PREFIX) - 1)

#ifndef FALSE
#	define FALSE 0
#endif

#ifndef TRUE
#	define TRUE !FALSE
#endif

#endif /* !TERRA_INTERNAL_H */
