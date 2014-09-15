#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
<<<<<<< HEAD
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
=======
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
>>>>>>> 062865a0cf25ef9331420f976c34d326c8196f3e
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
<<<<<<< HEAD
	{ 0x7dc216d9, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xfbbc9790, __VMLINUX_SYMBOL_STR(pv_cpu_ops) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xdcb0349b, __VMLINUX_SYMBOL_STR(sys_close) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x1e6d26a8, __VMLINUX_SYMBOL_STR(strstr) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0xa6327d27, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xdf08d9a5, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
=======
	{ 0x1792d969, "module_layout" },
	{ 0x63e3ee70, "pv_cpu_ops" },
	{ 0x27e1a049, "printk" },
	{ 0xdcb0349b, "sys_close" },
	{ 0x37a0cba, "kfree" },
	{ 0x1e6d26a8, "strstr" },
	{ 0x4f6b400b, "_copy_from_user" },
	{ 0xbfec4eb4, "kmem_cache_alloc_trace" },
	{ 0x868cc109, "kmalloc_caches" },
	{ 0xb4390f9a, "mcount" },
>>>>>>> 062865a0cf25ef9331420f976c34d326c8196f3e
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "DAAD5E80AA1D1C23F73D7C6");
