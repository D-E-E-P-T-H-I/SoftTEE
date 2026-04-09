#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/timekeeping.h>
#include <linux/version.h>

#include "../include/pex_uapi.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SofTEE");
MODULE_DESCRIPTION("Kernel-Assisted Protected Execution Subsystem");

struct pex_context {
    bool used;
    u32 id;
    char name[PEX_NAME_LEN];
    size_t size;
    unsigned long npages;
    struct page **pages;

    pid_t owner_tgid;
    pid_t owner_tid;

    bool active;
    u64 enter_count;
    u64 fault_count;
    u64 total_exec_ns;
    u64 last_enter_ns;
};

static struct pex_context g_ctx[PEX_MAX_CONTEXTS];
static DEFINE_MUTEX(g_lock);
static struct proc_dir_entry *g_proc;

static struct pex_context *ctx_get(u32 id)
{
    if (id >= PEX_MAX_CONTEXTS || !g_ctx[id].used)
        return NULL;
    return &g_ctx[id];
}

static int ctx_alloc_pages(struct pex_context *ctx)
{
    unsigned long i;

    ctx->pages = kcalloc(ctx->npages, sizeof(*ctx->pages), GFP_KERNEL);
    if (!ctx->pages)
        return -ENOMEM;

    for (i = 0; i < ctx->npages; i++) {
        ctx->pages[i] = alloc_page(GFP_KERNEL | __GFP_ZERO);
        if (!ctx->pages[i])
            goto err;
    }
    return 0;

err:
    while (i > 0)
        __free_page(ctx->pages[--i]);
    kfree(ctx->pages);
    ctx->pages = NULL;
    return -ENOMEM;
}

static void ctx_free_pages(struct pex_context *ctx)
{
    unsigned long i;

    if (!ctx->pages)
        return;

    for (i = 0; i < ctx->npages; i++) {
        if (ctx->pages[i])
            __free_page(ctx->pages[i]);
    }

    kfree(ctx->pages);
    ctx->pages = NULL;
}

static vm_fault_t pex_vm_fault(struct vm_fault *vmf)
{
    struct vm_area_struct *vma = vmf->vma;
    struct pex_context *ctx = vma->vm_private_data;
    unsigned long page_idx;
    struct page *page;

    if (!ctx)
        return VM_FAULT_SIGBUS;

    mutex_lock(&g_lock);

    if (!ctx->used) {
        mutex_unlock(&g_lock);
        return VM_FAULT_SIGBUS;
    }

    if (!ctx->active || current->pid != ctx->owner_tid || current->tgid != ctx->owner_tgid) {
        ctx->fault_count++;
        mutex_unlock(&g_lock);
        return VM_FAULT_SIGSEGV;
    }

    page_idx = vmf->pgoff;
    if (page_idx >= ctx->npages) {
        mutex_unlock(&g_lock);
        return VM_FAULT_SIGBUS;
    }

    page = ctx->pages[page_idx];
    get_page(page);
    vmf->page = page;

    mutex_unlock(&g_lock);
    return 0;
}

static const struct vm_operations_struct pex_vm_ops = {
    .fault = pex_vm_fault,
};

static int pex_mmap(struct file *filp, struct vm_area_struct *vma)
{
    u32 id = (u32)vma->vm_pgoff;
    struct pex_context *ctx;
    unsigned long req_size = vma->vm_end - vma->vm_start;

    mutex_lock(&g_lock);

    ctx = ctx_get(id);
    if (!ctx) {
        mutex_unlock(&g_lock);
        return -ENOENT;
    }

    if (ctx->owner_tgid != current->tgid) {
        mutex_unlock(&g_lock);
        return -EPERM;
    }

    if (req_size > ctx->size) {
        mutex_unlock(&g_lock);
        return -EINVAL;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)
    vm_flags_set(vma, VM_DONTDUMP | VM_DONTCOPY);
#else
    vma->vm_flags |= VM_DONTDUMP | VM_DONTCOPY;
#endif
    vma->vm_private_data = ctx;
    vma->vm_ops = &pex_vm_ops;

    mutex_unlock(&g_lock);
    return 0;
}

static long pex_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct pex_create_req creq;
    struct pex_id_req ireq;
    struct pex_stats_req sreq;
    struct pex_context *ctx;
    u64 now;
    int i;

    switch (cmd) {
    case PEX_IOCTL_CREATE:
        if (copy_from_user(&creq, (void __user *)arg, sizeof(creq)))
            return -EFAULT;

        if (!creq.size)
            return -EINVAL;

        creq.size = PAGE_ALIGN(creq.size);

        mutex_lock(&g_lock);
        for (i = 0; i < PEX_MAX_CONTEXTS; i++) {
            if (!g_ctx[i].used)
                break;
        }
        if (i == PEX_MAX_CONTEXTS) {
            mutex_unlock(&g_lock);
            return -ENOSPC;
        }

        memset(&g_ctx[i], 0, sizeof(g_ctx[i]));
        g_ctx[i].used = true;
        g_ctx[i].id = i;
        g_ctx[i].size = creq.size;
        g_ctx[i].npages = creq.size >> PAGE_SHIFT;
        g_ctx[i].owner_tgid = current->tgid;
        g_ctx[i].owner_tid = current->pid;
        strscpy(g_ctx[i].name, creq.name, sizeof(g_ctx[i].name));

        if (ctx_alloc_pages(&g_ctx[i])) {
            memset(&g_ctx[i], 0, sizeof(g_ctx[i]));
            mutex_unlock(&g_lock);
            return -ENOMEM;
        }

        creq.id = i;
        mutex_unlock(&g_lock);

        if (copy_to_user((void __user *)arg, &creq, sizeof(creq)))
            return -EFAULT;

        return 0;

    case PEX_IOCTL_DESTROY:
        if (copy_from_user(&ireq, (void __user *)arg, sizeof(ireq)))
            return -EFAULT;

        mutex_lock(&g_lock);
        ctx = ctx_get(ireq.id);
        if (!ctx) {
            mutex_unlock(&g_lock);
            return -ENOENT;
        }
        if (ctx->owner_tgid != current->tgid) {
            mutex_unlock(&g_lock);
            return -EPERM;
        }

        ctx_free_pages(ctx);
        memset(ctx, 0, sizeof(*ctx));
        mutex_unlock(&g_lock);
        return 0;

    case PEX_IOCTL_ENTER:
        if (copy_from_user(&ireq, (void __user *)arg, sizeof(ireq)))
            return -EFAULT;

        mutex_lock(&g_lock);
        ctx = ctx_get(ireq.id);
        if (!ctx) {
            mutex_unlock(&g_lock);
            return -ENOENT;
        }
        if (ctx->owner_tid != current->pid || ctx->owner_tgid != current->tgid) {
            mutex_unlock(&g_lock);
            return -EPERM;
        }
        if (ctx->active) {
            mutex_unlock(&g_lock);
            return -EBUSY;
        }

        ctx->active = true;
        ctx->enter_count++;
        ctx->last_enter_ns = ktime_get_ns();
        mutex_unlock(&g_lock);
        return 0;

    case PEX_IOCTL_EXIT:
        if (copy_from_user(&ireq, (void __user *)arg, sizeof(ireq)))
            return -EFAULT;

        mutex_lock(&g_lock);
        ctx = ctx_get(ireq.id);
        if (!ctx) {
            mutex_unlock(&g_lock);
            return -ENOENT;
        }
        if (ctx->owner_tid != current->pid || ctx->owner_tgid != current->tgid) {
            mutex_unlock(&g_lock);
            return -EPERM;
        }
        if (!ctx->active) {
            mutex_unlock(&g_lock);
            return -EINVAL;
        }

        now = ktime_get_ns();
        ctx->active = false;
        ctx->total_exec_ns += (now - ctx->last_enter_ns);
        ctx->last_enter_ns = 0;
        mutex_unlock(&g_lock);
        return 0;

    case PEX_IOCTL_STATS:
        if (copy_from_user(&sreq, (void __user *)arg, sizeof(sreq)))
            return -EFAULT;

        mutex_lock(&g_lock);
        ctx = ctx_get(sreq.id);
        if (!ctx) {
            mutex_unlock(&g_lock);
            return -ENOENT;
        }

        sreq.owner_tgid = ctx->owner_tgid;
        sreq.owner_tid = ctx->owner_tid;
        sreq.active = ctx->active;
        sreq.size = ctx->size;
        sreq.enter_count = ctx->enter_count;
        sreq.fault_count = ctx->fault_count;
        sreq.total_exec_ns = ctx->total_exec_ns;

        mutex_unlock(&g_lock);

        if (copy_to_user((void __user *)arg, &sreq, sizeof(sreq)))
            return -EFAULT;

        return 0;

    default:
        return -ENOTTY;
    }
}

static int pex_proc_show(struct seq_file *m, void *v)
{
    int i;

    seq_puts(m, "id name owner_tgid owner_tid active size enter_count fault_count total_exec_ns\n");

    mutex_lock(&g_lock);
    for (i = 0; i < PEX_MAX_CONTEXTS; i++) {
        struct pex_context *ctx = &g_ctx[i];
        if (!ctx->used)
            continue;

        seq_printf(m, "%u %s %u %u %u %llu %llu %llu %llu\n",
                   ctx->id,
                   ctx->name[0] ? ctx->name : "unnamed",
                   ctx->owner_tgid,
                   ctx->owner_tid,
                   ctx->active,
                   (unsigned long long)ctx->size,
                   (unsigned long long)ctx->enter_count,
                   (unsigned long long)ctx->fault_count,
                   (unsigned long long)ctx->total_exec_ns);
    }
    mutex_unlock(&g_lock);

    return 0;
}

static int pex_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, pex_proc_show, NULL);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops pex_proc_ops = {
    .proc_open = pex_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations pex_proc_ops = {
    .owner = THIS_MODULE,
    .open = pex_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
#endif

static const struct file_operations pex_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = pex_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = pex_ioctl,
#endif
    .mmap = pex_mmap,
};

static struct miscdevice pex_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "pex",
    .fops = &pex_fops,
    .mode = 0666,
};

static int __init pex_init(void)
{
    int ret;

    ret = misc_register(&pex_misc);
    if (ret)
        return ret;

    g_proc = proc_create("pex_stats", 0444, NULL, &pex_proc_ops);
    if (!g_proc) {
        misc_deregister(&pex_misc);
        return -ENOMEM;
    }

    pr_info("pex: module loaded\n");
    return 0;
}

static void __exit pex_exit(void)
{
    int i;

    mutex_lock(&g_lock);
    for (i = 0; i < PEX_MAX_CONTEXTS; i++) {
        if (g_ctx[i].used) {
            ctx_free_pages(&g_ctx[i]);
            memset(&g_ctx[i], 0, sizeof(g_ctx[i]));
        }
    }
    mutex_unlock(&g_lock);

    if (g_proc)
        proc_remove(g_proc);
    misc_deregister(&pex_misc);
    pr_info("pex: module unloaded\n");
}

module_init(pex_init);
module_exit(pex_exit);