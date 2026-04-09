#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "../libpex/pex.h"

struct thread_arg {
    volatile char *ptr;
};

static void *cross_thread_probe(void *arg)
{
    struct thread_arg *a = (struct thread_arg *)arg;
    printf("[worker] trying cross-thread read (should segfault) ...\n");
    fflush(stdout);
    (void)a->ptr[0];
    return NULL;
}

int main(void)
{
    int fd;
    pex_ctx_t ctx;
    struct pex_stats_req st;
    pthread_t t;
    struct thread_arg arg;

    fd = pex_open();
    if (fd < 0) {
        perror("pex_open");
        return 1;
    }

    if (pex_create(fd, 4096, "demo", &ctx) < 0) {
        perror("pex_create");
        return 1;
    }

    if (pex_map(&ctx) < 0) {
        perror("pex_map");
        return 1;
    }

    printf("created context id=%u size=%zu\n", ctx.id, ctx.size);

    if (pex_enter(&ctx) < 0) {
        perror("pex_enter");
        return 1;
    }

    strcpy((char *)ctx.addr, "secret:kernel-assisted-pex");
    printf("inside pex: wrote secret: %s\n", (char *)ctx.addr);

    if (pex_exit(&ctx) < 0) {
        perror("pex_exit");
        return 1;
    }

    printf("outside pex: next line intentionally triggers segfault if enforcement works.\n");
    fflush(stdout);

    arg.ptr = (volatile char *)ctx.addr;
    pthread_create(&t, NULL, cross_thread_probe, &arg);
    pthread_join(t, NULL);

    if (pex_stats(fd, ctx.id, &st) == 0) {
        printf("stats: enter=%llu faults=%llu total_ns=%llu active=%u\n",
               (unsigned long long)st.enter_count,
               (unsigned long long)st.fault_count,
               (unsigned long long)st.total_exec_ns,
               st.active);
    }

    pex_destroy(fd, ctx.id);
    pex_close(fd);
    return 0;
}