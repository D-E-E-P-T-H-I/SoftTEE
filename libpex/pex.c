#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include "pex.h"

int pex_open(void)
{
    return open("/dev/pex", O_RDWR);
}

int pex_close(int fd)
{
    return close(fd);
}

int pex_create(int fd, size_t size, const char *name, pex_ctx_t *out)
{
    struct pex_create_req req = {
        .size = size,
    };

    if (!out) {
        errno = EINVAL;
        return -1;
    }

    if (name) {
        strncpy(req.name, name, sizeof(req.name) - 1);
        req.name[sizeof(req.name) - 1] = '\0';
    }

    if (ioctl(fd, PEX_IOCTL_CREATE, &req) < 0)
        return -1;

    memset(out, 0, sizeof(*out));
    out->fd = fd;
    out->id = req.id;
    out->size = req.size;
    return 0;
}

int pex_map(pex_ctx_t *ctx)
{
    off_t off;

    if (!ctx || !ctx->size) {
        errno = EINVAL;
        return -1;
    }

    off = (off_t)ctx->id * (off_t)4096;
    ctx->addr = mmap(NULL, ctx->size, PROT_READ | PROT_WRITE, MAP_SHARED, ctx->fd, off);
    if (ctx->addr == MAP_FAILED) {
        ctx->addr = NULL;
        return -1;
    }

    return 0;
}

int pex_enter(pex_ctx_t *ctx)
{
    struct pex_id_req req;
    if (!ctx) {
        errno = EINVAL;
        return -1;
    }

    req.id = ctx->id;
    req.reserved = 0;
    return ioctl(ctx->fd, PEX_IOCTL_ENTER, &req);
}

int pex_exit(pex_ctx_t *ctx)
{
    struct pex_id_req req;
    if (!ctx) {
        errno = EINVAL;
        return -1;
    }

    req.id = ctx->id;
    req.reserved = 0;
    return ioctl(ctx->fd, PEX_IOCTL_EXIT, &req);
}

int pex_stats(int fd, uint32_t id, struct pex_stats_req *out)
{
    struct pex_stats_req req;
    memset(&req, 0, sizeof(req));
    req.id = id;

    if (ioctl(fd, PEX_IOCTL_STATS, &req) < 0)
        return -1;

    if (out)
        *out = req;

    return 0;
}

int pex_destroy(int fd, uint32_t id)
{
    struct pex_id_req req;
    req.id = id;
    req.reserved = 0;
    return ioctl(fd, PEX_IOCTL_DESTROY, &req);
}