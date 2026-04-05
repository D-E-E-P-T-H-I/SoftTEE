#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../libpex/include/pex.h"

static pex_handle_t g_handle;

static void *thread_fn(void *arg)
{
    int rc = pex_enter(&g_handle);
    printf("secondary thread pex_enter rc=%d (expected < 0)\n", rc);
    (void)arg;
    return NULL;
}

int main(void)
{
    pthread_t t;
    int rc;

    rc = pex_open(&g_handle);
    if (rc) return 1;
    rc = pex_create(&g_handle, "thread_test", 4096, PEX_POLICY_OWNER_THREAD_ONLY);
    if (rc) return 1;

    rc = pthread_create(&t, NULL, thread_fn, NULL);
    if (rc) return 1;
    pthread_join(t, NULL);

    pex_destroy(&g_handle);
    pex_close(&g_handle);
    return 0;
}
