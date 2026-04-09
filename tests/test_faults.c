#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>

#include "../libpex/pex.h"

static sigjmp_buf g_jmp;

static void on_segv(int sig)
{
    (void)sig;
    siglongjmp(g_jmp, 1);
}

int main(void)
{
    int fd;
    pex_ctx_t ctx;
    struct sigaction sa = {0};

    sa.sa_handler = on_segv;
    sigaction(SIGSEGV, &sa, NULL);

    fd = pex_open();
    if (fd < 0) {
        perror("pex_open");
        return 1;
    }

    if (pex_create(fd, 4096, "fault-test", &ctx) < 0) {
        perror("pex_create");
        return 1;
    }

    if (pex_map(&ctx) < 0) {
        perror("pex_map");
        return 1;
    }

    if (sigsetjmp(g_jmp, 1) == 0) {
        volatile char c = *((volatile char *)ctx.addr);
        (void)c;
        printf("FAIL: read outside protected mode unexpectedly succeeded\n");
    } else {
        printf("PASS: fault captured for outside access\n");
    }

    if (pex_enter(&ctx) < 0) {
        perror("pex_enter");
        return 1;
    }

    strcpy((char *)ctx.addr, "ok");
    printf("PASS: inside access succeeded (%s)\n", (char *)ctx.addr);

    pex_exit(&ctx);
    pex_destroy(fd, ctx.id);
    pex_close(fd);
    return 0;
}