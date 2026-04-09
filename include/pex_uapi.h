#ifndef PEX_UAPI_H
#define PEX_UAPI_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define PEX_MAX_CONTEXTS 64
#define PEX_NAME_LEN 32

struct pex_create_req {
    __u64 size;
    __u32 flags;
    __u32 id;
    char name[PEX_NAME_LEN];
};

struct pex_id_req {
    __u32 id;
    __u32 reserved;
};

struct pex_stats_req {
    __u32 id;
    __u32 owner_tgid;
    __u32 owner_tid;
    __u32 active;
    __u64 size;
    __u64 enter_count;
    __u64 fault_count;
    __u64 total_exec_ns;
};

#define PEX_IOCTL_MAGIC 'P'
#define PEX_IOCTL_CREATE _IOWR(PEX_IOCTL_MAGIC, 1, struct pex_create_req)
#define PEX_IOCTL_DESTROY _IOW(PEX_IOCTL_MAGIC, 2, struct pex_id_req)
#define PEX_IOCTL_ENTER _IOW(PEX_IOCTL_MAGIC, 3, struct pex_id_req)
#define PEX_IOCTL_EXIT _IOW(PEX_IOCTL_MAGIC, 4, struct pex_id_req)
#define PEX_IOCTL_STATS _IOWR(PEX_IOCTL_MAGIC, 5, struct pex_stats_req)

#endif