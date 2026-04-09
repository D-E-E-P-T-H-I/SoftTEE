#ifndef LIBPEX_H
#define LIBPEX_H

#include <stddef.h>
#include <stdint.h>

#include "../include/pex_uapi.h"

typedef struct pex_ctx {
    int fd;
    uint32_t id;
    size_t size;
    void *addr;
} pex_ctx_t;

int pex_open(void);
int pex_close(int fd);
int pex_create(int fd, size_t size, const char *name, pex_ctx_t *out);
int pex_map(pex_ctx_t *ctx);
int pex_enter(pex_ctx_t *ctx);
int pex_exit(pex_ctx_t *ctx);
int pex_stats(int fd, uint32_t id, struct pex_stats_req *out);
int pex_destroy(int fd, uint32_t id);

#endif