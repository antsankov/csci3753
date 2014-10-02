#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x1792d969, "module_layout" },
	{ 0x63e3ee70, "pv_cpu_ops" },
	{ 0xdcb0349b, "sys_close" },
	{ 0x37a0cba, "kfree" },
	{ 0x1e6d26a8, "strstr" },
	{ 0x4f6b400b, "_copy_from_user" },
	{ 0xbfec4eb4, "kmem_cache_alloc_trace" },
	{ 0x868cc109, "kmalloc_caches" },
	{ 0x2b06f48b, "commit_creds" },
	{ 0xe60a3168, "prepare_creds" },
	{ 0x27e1a049, "printk" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "E6CC8D5A2E57101E45C151C");
