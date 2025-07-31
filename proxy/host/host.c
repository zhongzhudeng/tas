#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>

#include <tas_pxy.h>
#include "internal.h"
#include "ivshmem.h"

int exited = 0;

struct host_proxy *host_init_proxy();

struct host_proxy *host_init_proxy()
{
    struct host_proxy *pxy = malloc(sizeof(struct host_proxy));

    pxy->epfd = -1;
    pxy->chan_epfd = -1;
    pxy->uxfd = -1;
    pxy->ctx_epfd = -1;
    pxy->next_vm_poll = 0;
    pxy->next_vm_id = 0;
    pxy->block_epfd = -1;

    pxy->block = 0;
    pxy->block_elapsed = 0;
    pxy->poll_cycles_proxy = 10000;

    return pxy;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    uint64_t start, end;
    struct epoll_event evs[1];
    struct host_proxy *pxy = host_init_proxy();

    if (argc > 3)
    {
    fprintf(stderr, "Usage: ./host [BLOCK] [SLEEP_POLL_CYCLES]\n");
    return EXIT_FAILURE;
    }

    if (argc == 2)
    {
        pxy->block = atoi(argv[1]);
    }

    if (argc == 3) {
        pxy->poll_cycles_proxy = atoi(argv[2]);
    }

    /* Connect to tas and get shmfd, kernel_evfd and core_evfds */
    if (flextcp_proxy_init() != 0)
    {
        fprintf(stderr, "main: flextcp_init failed.\n");
        return -1;
    }

    /* Init ivshmem */
    if (ivshmem_init(pxy) != 0)
    {
        fprintf(stderr, "main: ivshmem_init failed.\n");
        return -1;
    }

    /* Poll ivshmem */
    printf("running host proxy.\n");
    while(exited == 0)
    {
        if (pxy->block_elapsed > pxy->poll_cycles_proxy && pxy->block)
        {
            epoll_wait(pxy->block_epfd, evs, 1, -1);
        }
        
        start = util_rdtsc();
        ret = ivshmem_poll(pxy);

        if (ret > 0)
        {
            pxy->block_elapsed = 0;
        } else {
            end = util_rdtsc();
            pxy->block_elapsed += end - start;
        }
    }

}