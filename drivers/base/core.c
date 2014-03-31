/*
 * drivers/base/core.c - core driver model code (device registration, etc)
 *
 * Copyright (c) 2002-3 Patrick Mochel
 * Copyright (c) 2002-3 Open Source Development Labs
 * Copyright (c) 2006 Greg Kroah-Hartman <gregkh@suse.de>
 * Copyright (c) 2006 Novell, Inc.
 *
 * This file is released under the GPLv2
 *
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kdev_t.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/genhd.h>
#include <linux/kallsyms.h>
#include <linux/mutex.h>
#include <linux/async.h>
#include <linux/pm_runtime.h>

#include "base.h"
#include "power/power.h"

#ifdef CONFIG_SYSFS_DEPRECATED
#ifdef CONFIG_SYSFS_DEPRECATED_V2
long sysfs_deprecated = 1;
#else
long sysfs_deprecated = 0;
#endif
static __init int sysfs_deprecated_setup(char *arg)
{
	return strict_strtol(arg, 10, &sysfs_deprecated);
}
early_param("sysfs.deprecated", sysfs_deprecated_setup);
#endif

int (*platform_notify)(struct device *dev) = NULL;
int (*platform_notify_remove)(struct device *dev) = NULL;
static struct kobject *dev_kobj;
struct kobject *sysfs_dev_char_kobj;
struct kobject *sysfs_dev_block_kobj;

#ifdef CONFIG_BLOCK
static inline int device_is_not_partition(struct device *dev)
{
	return !(dev->type == &part_type);
}
#else
static inline int device_is_not_partition(struct device *dev)
{
	return 1;
}
#endif

const char *dev_driver_string(const struct device *dev)
{
	struct device_driver *drv;

	drv = ACCESS_ONCE(dev->driver);
	return drv ? drv->name :
			(dev->bus ? dev->bus->name :
			(dev->class ? dev->class->name : ""));
}
EXPORT_SYMBOL(dev_driver_string);

#define to_dev(obj) container_of(obj, struct device, kobj)
#define to_dev_attr(_attr) container_of(_attr, struct device_attribute, attr)

static ssize_t dev_attr_show(struct kobject *kobj, struct attribute *attr,
			     char *buf)
{
	struct device_attribute *dev_attr = to_dev_attr(attr);
	struct device *dev = to_dev(kobj);
	ssize_t ret = -EIO;

	if (dev_attr->show)
		ret = dev_attr->show(dev, dev_attr, buf);
	if (ret >= (ssize_t)PAGE_SIZE) {
		print_symbol("dev_attr_show: %s returned bad count\n",
				(unsigned long)dev_attr->show);
	}
	return ret;
}

static ssize_t dev_attr_store(struct kobject *kobj, struct attribute *attr,
			      const char *buf, size_t count)
{
	struct device_attribute *dev_attr = to_dev_attr(attr);
	struct device *dev = to_dev(kobj);
	ssize_t ret = -EIO;

	if (dev_attr->store)
		ret = dev_attr->store(dev, dev_attr, buf, count);
	return ret;
}

static const struct sysfs_ops dev_sysfs_ops = {
	.show	= dev_attr_show,
	.store	= dev_attr_store,
};

#define to_ext_attr(x) container_of(x, struct dev_ext_attribute, attr)

ssize_t device_store_ulong(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t size)
{
	struct dev_ext_attribute *ea = to_ext_attr(attr);
	char *end;
	unsigned long new = simple_strtoul(buf, &end, 0);
	if (end == buf)
		return -EINVAL;
	*(unsigned long *)(ea->var) = new;
	
	return size;
}
EXPORT_SYMBOL_GPL(device_store_ulong);

ssize_t device_show_ulong(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	struct dev_ext_attribute *ea = to_ext_attr(attr);
	return snprintf(buf, PAGE_SIZE, "%lx\n", *(unsigned long *)(ea->var));
}
EXPORT_SYMBOL_GPL(device_show_ulong);

ssize_t device_store_int(struct device *dev,
			 struct device_attribute *attr,
			 const char *buf, size_t size)
{
	struct dev_ext_attribute *ea = to_ext_attr(attr);
	char *end;
	long new = simple_strtol(buf, &end, 0);
	if (end == buf || new > INT_MAX || new < INT_MIN)
		return -EINVAL;
	*(int *)(ea->var) = new;
	
	return size;
}
EXPORT_SYMBOL_GPL(device_store_int);

ssize_t device_show_int(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	struct dev_ext_attribute *ea = to_ext_attr(attr);

	return snprintf(buf, PAGE_SIZE, "%d\n", *(int *)(ea->var));
}
EXPORT_SYMBOL_GPL(device_show_int);

static void device_release(struct kobject *kobj)
{
	struct device *dev = to_dev(kobj);
	struct device_private *p = dev->p;

	if (dev->release)
		dev->release(dev);
	else if (dev->type && dev->type->release)
		dev->type->release(dev);
	else if (dev->class && dev->class->dev_release)
		dev->class->dev_release(dev);
	else
		WARN(1, KERN_ERR "Device '%s' does not have a release() "
			"function, it is broken and must be fixed.\n",
			dev_name(dev));
	kfree(p);
}

static const void *device_namespace(struct kobject *kobj)
{
	struct device *dev = to_dev(kobj);
	const void *ns = NULL;

	if (dev->class && dev->class->ns_type)
		ns = dev->class->namespace(dev);

	return ns;
}

static struct kobj_type device_ktype = {
	.release	= device_release,
	.sysfs_ops	= &dev_sysfs_ops,
	.namespace	= device_namespace,
};


static int dev_uevent_filter(struct kset *kset, struct kobject *kobj)
{
	struct kobj_type *ktype = get_ktype(kobj);

	if (ktype == &device_ktype) {
		struct device *dev = to_dev(kobj);
		if (dev->bus)
			return 1;
		if (dev->class)
			return 1;
	}
	return 0;
}

static const char *dev_uevent_name(struct kset *kset, struct kobject *kobj)
{
	struct device *dev = to_dev(kobj);

	if (dev->bus)
		return dev->bus->name;
	if (dev->class)
		return dev->class->name;
	return NULL;
}

static int dev_uevent(struct kset *kset, struct kobject *kobj,
		      struct kobj_uevent_env *env)
{
	struct device *dev = to_dev(kobj);
	int retval = 0;

	
	if (MAJOR(dev->devt)) {
		const char *tmp;
		const char *name;
		umode_t mode = 0;

		add_uevent_var(env, "MAJOR=%u", MAJOR(dev->devt));
		add_uevent_var(env, "MINOR=%u", MINOR(dev->devt));
		name = device_get_devnode(dev, &mode, &tmp);
		if (name) {
			add_uevent_var(env, "DEVNAME=%s", name);
			kfree(tmp);
			if (mode)
				add_uevent_var(env, "DEVMODE=%#o", mode & 0777);
		}
	}

	if (dev->type && dev->type->name)
		add_uevent_var(env, "DEVTYPE=%s", dev->type->name);

	if (dev->driver)
		add_uevent_var(env, "DRIVER=%s", dev->driver->name);

	
	of_device_uevent(dev, env);

	
	if (dev->bus && dev->bus->uevent) {
		retval = dev->bus->uevent(dev, env);
		if (retval)
			pr_debug("device: '%s': %s: bus uevent() returned %d\n",
				 dev_name(dev), __func__, retval);
	}

	
	if (dev->class && dev->class->dev_uevent) {
		retval = dev->class->dev_uevent(dev, env);
		if (retval)
			pr_debug("device: '%s': %s: class uevent() "
				 "returned %d\n", dev_name(dev),
				 __func__, retval);
	}

	
	if (dev->type && dev->type->uevent) {
		retval = dev->type->uevent(dev, env);
		if (retval)
			pr_debug("device: '%s': %s: dev_type uevent() "
				 "returned %d\n", dev_name(dev),
				 __func__, retval);
	}

	return retval;
}

static const struct kset_uevent_ops device_uevent_ops = {
	.filter =	dev_uevent_filter,
	.name =		dev_uevent_name,
	.uevent =	dev_uevent,
};

static ssize_t show_uevent(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct kobject *top_kobj;
	struct kset *kset;
	struct kobj_uevent_env *env = NULL;
	int i;
	size_t count = 0;
	int retval;

	
	top_kobj = &dev->kobj;
	while (!top_kobj->kset && top_kobj->parent)
		top_kobj = top_kobj->parent;
	if (!top_kobj->kset)
		goto out;

	kset = top_kobj->kset;
	if (!kset->uevent_ops || !kset->uevent_ops->uevent)
		goto out;

	
	if (kset->uevent_ops && kset->uevent_ops->filter)
		if (!kset->uevent_ops->filter(kset, &dev->kobj))
			goto out;

	env = kzalloc(sizeof(struct kobj_uevent_env), GFP_KERNEL);
	if (!env)
		return -ENOMEM;

	
	retval = kset->uevent_ops->uevent(kset, &dev->kobj, env);
	if (retval)
		goto out;

	
	for (i = 0; i < env->envp_idx; i++)
		count += sprintf(&buf[count], "%s\n", env->envp[i]);
out:
	kfree(env);
	return count;
}

static ssize_t store_uevent(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	enum kobject_action action;

	if (kobject_action_type(buf, count, &action) == 0)
		kobject_uevent(&dev->kobj, action);
	else
		dev_err(dev, "uevent: unknown action-string\n");
	return count;
}

static struct device_attribute uevent_attr =
	__ATTR(uevent, S_IRUGO | S_IWUSR, show_uevent, store_uevent);

static int device_add_attributes(struct device *dev,
				 struct device_attribute *attrs)
{
	int error = 0;
	int i;

	if (attrs) {
		for (i = 0; attr_name(attrs[i]); i++) {
			error = device_create_file(dev, &attrs[i]);
			if (error)
				break;
		}
		if (error)
			while (--i >= 0)
				device_remove_file(dev, &attrs[i]);
	}
	return error;
}

static void device_remove_attributes(struct device *dev,
				     struct device_attribute *attrs)
{
	int i;

	if (attrs)
		for (i = 0; attr_name(attrs[i]); i++)
			device_remove_file(dev, &attrs[i]);
}

static int device_add_bin_attributes(struct device *dev,
				     struct bin_attribute *attrs)
{
	int error = 0;
	int i;

	if (attrs) {
		for (i = 0; attr_name(attrs[i]); i++) {
			error = device_create_bin_file(dev, &attrs[i]);
			if (error)
				break;
		}
		if (error)
			while (--i >= 0)
				device_remove_bin_file(dev, &attrs[i]);
	}
	return error;
}

static void device_remove_bin_attributes(struct device *dev,
					 struct bin_attribute *attrs)
{
	int i;

	if (attrs)
		for (i = 0; attr_name(attrs[i]); i++)
			device_remove_bin_file(dev, &attrs[i]);
}

static int device_add_groups(struct device *dev,
			     const struct attribute_group **groups)
{
	int error = 0;
	int i;

	if (groups) {
		for (i = 0; groups[i]; i++) {
			error = sysfs_create_group(&dev->kobj, groups[i]);
			if (error) {
				while (--i >= 0)
					sysfs_remove_group(&dev->kobj,
							   groups[i]);
				break;
			}
		}
	}
	return error;
}

static void device_remove_groups(struct device *dev,
				 const struct attribute_group **groups)
{
	int i;

	if (groups)
		for (i = 0; groups[i]; i++)
			sysfs_remove_group(&dev->kobj, groups[i]);
}

static int device_add_attrs(struct device *dev)
{
	struct class *class = dev->class;
	const struct device_type *type = dev->type;
	int error;

	if (class) {
		error = device_add_attributes(dev, class->dev_attrs);
		if (error)
			return error;
		error = device_add_bin_attributes(dev, class->dev_bin_attrs);
		if (error)
			goto err_remove_class_attrs;
	}

	if (type) {
		error = device_add_groups(dev, type->groups);
		if (error)
			goto err_remove_class_bin_attrs;
	}

	error = device_add_groups(dev, dev->groups);
	if (error)
		goto err_remove_type_groups;

	return 0;

 err_remove_type_groups:
	if (type)
		device_remove_groups(dev, type->groups);
 err_remove_class_bin_attrs:
	if (class)
		device_remove_bin_attributes(dev, class->dev_bin_attrs);
 err_remove_class_attrs:
	if (class)
		device_remove_attributes(dev, class->dev_attrs);

	return error;
}

static void device_remove_attrs(struct device *dev)
{
	struct class *class = dev->class;
	const struct device_type *type = dev->type;

	device_remove_groups(dev, dev->groups);

	if (type)
		device_remove_groups(dev, type->groups);

	if (class) {
		device_remove_attributes(dev, class->dev_attrs);
		device_remove_bin_attributes(dev, class->dev_bin_attrs);
	}
}


static ssize_t show_dev(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return print_dev_t(buf, dev->devt);
}

static struct device_attribute devt_attr =
	__ATTR(dev, S_IRUGO, show_dev, NULL);

struct kset *devices_kset;

int device_create_file(struct device *dev,
		       const struct device_attribute *attr)
{
	int error = 0;
	if (dev)
		error = sysfs_create_file(&dev->kobj, &attr->attr);
	return error;
}

void device_remove_file(struct device *dev,
			const struct device_attribute *attr)
{
	if (dev)
		sysfs_remove_file(&dev->kobj, &attr->attr);
}

int device_create_bin_file(struct device *dev,
			   const struct bin_attribute *attr)
{
	int error = -EINVAL;
	if (dev)
		error = sysfs_create_bin_file(&dev->kobj, attr);
	return error;
}
EXPORT_SYMBOL_GPL(device_create_bin_file);

void device_remove_bin_file(struct device *dev,
			    const struct bin_attribute *attr)
{
	if (dev)
		sysfs_remove_bin_file(&dev->kobj, attr);
}
EXPORT_SYMBOL_GPL(device_remove_bin_file);

int device_schedule_callback_owner(struct device *dev,
		void (*func)(struct device *), struct module *owner)
{
	return sysfs_schedule_callback(&dev->kobj,
			(void (*)(void *)) func, dev, owner);
}
EXPORT_SYMBOL_GPL(device_schedule_callback_owner);

static void klist_children_get(struct klist_node *n)
{
	struct device_private *p = to_device_private_parent(n);
	struct device *dev = p->device;

	get_device(dev);
}

static void klist_children_put(struct klist_node *n)
{
	struct device_private *p = to_device_private_parent(n);
	struct device *dev = p->device;

	put_device(dev);
}

void device_initialize(struct device *dev)
{
	dev->kobj.kset = devices_kset;
	kobject_init(&dev->kobj, &device_ktype);
	INIT_LIST_HEAD(&dev->dma_pools);
	mutex_init(&dev->mutex);
	lockdep_set_novalidate_class(&dev->mutex);
	spin_lock_init(&dev->devres_lock);
	INIT_LIST_HEAD(&dev->devres_head);
	device_pm_init(dev);
	set_dev_node(dev, -1);
}

static struct kobject *virtual_device_parent(struct device *dev)
{
	static struct kobject *virtual_dir = NULL;

	if (!virtual_dir)
		virtual_dir = kobject_create_and_add("virtual",
						     &devices_kset->kobj);

	return virtual_dir;
}

struct class_dir {
	struct kobject kobj;
	struct class *class;
};

#define to_class_dir(obj) container_of(obj, struct class_dir, kobj)

static void class_dir_release(struct kobject *kobj)
{
	struct class_dir *dir = to_class_dir(kobj);
	kfree(dir);
}

static const
struct kobj_ns_type_operations *class_dir_child_ns_type(struct kobject *kobj)
{
	struct class_dir *dir = to_class_dir(kobj);
	return dir->class->ns_type;
}

static struct kobj_type class_dir_ktype = {
	.release	= class_dir_release,
	.sysfs_ops	= &kobj_sysfs_ops,
	.child_ns_type	= class_dir_child_ns_type
};

static struct kobject *
class_dir_create_and_add(struct class *class, struct kobject *parent_kobj)
{
	struct class_dir *dir;
	int retval;

	dir = kzalloc(sizeof(*dir), GFP_KERNEL);
	if (!dir)
		return NULL;

	dir->class = class;
	kobject_init(&dir->kobj, &class_dir_ktype);

	dir->kobj.kset = &class->p->glue_dirs;

	retval = kobject_add(&dir->kobj, parent_kobj, "%s", class->name);
	if (retval < 0) {
		kobject_put(&dir->kobj);
		return NULL;
	}
	return &dir->kobj;
}


static struct kobject *get_device_parent(struct device *dev,
					 struct device *parent)
{
	if (dev->class) {
		static DEFINE_MUTEX(gdp_mutex);
		struct kobject *kobj = NULL;
		struct kobject *parent_kobj;
		struct kobject *k;

#ifdef CONFIG_BLOCK
		
		if (sysfs_deprecated && dev->class == &block_class) {
			if (parent && parent->class == &block_class)
				return &parent->kobj;
			return &block_class.p->subsys.kobj;
		}
#endif

		if (parent == NULL)
			parent_kobj = virtual_device_parent(dev);
		else if (parent->class && !dev->class->ns_type)
			return &parent->kobj;
		else
			parent_kobj = &parent->kobj;

		mutex_lock(&gdp_mutex);

		
		spin_lock(&dev->class->p->glue_dirs.list_lock);
		list_for_each_entry(k, &dev->class->p->glue_dirs.list, entry)
			if (k->parent == parent_kobj) {
				kobj = kobject_get(k);
				break;
			}
		spin_unlock(&dev->class->p->glue_dirs.list_lock);
		if (kobj) {
			mutex_unlock(&gdp_mutex);
			return kobj;
		}

		
		k = class_dir_create_and_add(dev->class, parent_kobj);
		
		mutex_unlock(&gdp_mutex);
		return k;
	}

	
	if (!parent && dev->bus && dev->bus->dev_root)
		return &dev->bus->dev_root->kobj;

	if (parent)
		return &parent->kobj;
	return NULL;
}

static void cleanup_glue_dir(struct device *dev, struct kobject *glue_dir)
{
	
	if (!glue_dir || !dev->class ||
	    glue_dir->kset != &dev->class->p->glue_dirs)
		return;

	kobject_put(glue_dir);
}

static void cleanup_device_parent(struct device *dev)
{
	cleanup_glue_dir(dev, dev->kobj.parent);
}

static int device_add_class_symlinks(struct device *dev)
{
	int error;

	if (!dev->class)
		return 0;

	error = sysfs_create_link(&dev->kobj,
				  &dev->class->p->subsys.kobj,
				  "subsystem");
	if (error)
		goto out;

	if (dev->parent && device_is_not_partition(dev)) {
		error = sysfs_create_link(&dev->kobj, &dev->parent->kobj,
					  "device");
		if (error)
			goto out_subsys;
	}

#ifdef CONFIG_BLOCK
	
	if (sysfs_deprecated && dev->class == &block_class)
		return 0;
#endif

	
	error = sysfs_create_link(&dev->class->p->subsys.kobj,
				  &dev->kobj, dev_name(dev));
	if (error)
		goto out_device;

	return 0;

out_device:
	sysfs_remove_link(&dev->kobj, "device");

out_subsys:
	sysfs_remove_link(&dev->kobj, "subsystem");
out:
	return error;
}

static void device_remove_class_symlinks(struct device *dev)
{
	if (!dev->class)
		return;

	if (dev->parent && device_is_not_partition(dev))
		sysfs_remove_link(&dev->kobj, "device");
	sysfs_remove_link(&dev->kobj, "subsystem");
#ifdef CONFIG_BLOCK
	if (sysfs_deprecated && dev->class == &block_class)
		return;
#endif
	sysfs_delete_link(&dev->class->p->subsys.kobj, &dev->kobj, dev_name(dev));
}

int dev_set_name(struct device *dev, const char *fmt, ...)
{
	va_list vargs;
	int err;

	va_start(vargs, fmt);
	err = kobject_set_name_vargs(&dev->kobj, fmt, vargs);
	va_end(vargs);
	return err;
}
EXPORT_SYMBOL_GPL(dev_set_name);

static struct kobject *device_to_dev_kobj(struct device *dev)
{
	struct kobject *kobj;

	if (dev->class)
		kobj = dev->class->dev_kobj;
	else
		kobj = sysfs_dev_char_kobj;

	return kobj;
}

static int device_create_sys_dev_entry(struct device *dev)
{
	struct kobject *kobj = device_to_dev_kobj(dev);
	int error = 0;
	char devt_str[15];

	if (kobj) {
		format_dev_t(devt_str, dev->devt);
		error = sysfs_create_link(kobj, &dev->kobj, devt_str);
	}

	return error;
}

static void device_remove_sys_dev_entry(struct device *dev)
{
	struct kobject *kobj = device_to_dev_kobj(dev);
	char devt_str[15];

	if (kobj) {
		format_dev_t(devt_str, dev->devt);
		sysfs_remove_link(kobj, devt_str);
	}
}

int device_private_init(struct device *dev)
{
	dev->p = kzalloc(sizeof(*dev->p), GFP_KERNEL);
	if (!dev->p)
		return -ENOMEM;
	dev->p->device = dev;
	klist_init(&dev->p->klist_children, klist_children_get,
		   klist_children_put);
	INIT_LIST_HEAD(&dev->p->deferred_probe);
	return 0;
}

int device_add(struct device *dev)
{
	struct device *parent = NULL;
	struct kobject *kobj;
	struct class_interface *class_intf;
	int error = -EINVAL;

	dev = get_device(dev);
	if (!dev)
		goto done;

	if (!dev->p) {
		error = device_private_init(dev);
		if (error)
			goto done;
	}

	if (dev->init_name) {
		dev_set_name(dev, "%s", dev->init_name);
		dev->init_name = NULL;
	}

	
	if (!dev_name(dev) && dev->bus && dev->bus->dev_name)
		dev_set_name(dev, "%s%u", dev->bus->dev_name, dev->id);

	if (!dev_name(dev)) {
		error = -EINVAL;
		goto name_error;
	}

	pr_debug("device: '%s': %s\n", dev_name(dev), __func__);

	parent = get_device(dev->parent);
	kobj = get_device_parent(dev, parent);
	if (kobj)
		dev->kobj.parent = kobj;

	
	if (parent)
		set_dev_node(dev, dev_to_node(parent));

	
	
	error = kobject_add(&dev->kobj, dev->kobj.parent, NULL);
	if (error)
		goto Error;

	
	if (platform_notify)
		platform_notify(dev);

	error = device_create_file(dev, &uevent_attr);
	if (error)
		goto attrError;

	if (MAJOR(dev->devt)) {
		error = device_create_file(dev, &devt_attr);
		if (error)
			goto ueventattrError;

		error = device_create_sys_dev_entry(dev);
		if (error)
			goto devtattrError;

		devtmpfs_create_node(dev);
	}

	error = device_add_class_symlinks(dev);
	if (error)
		goto SymlinkError;
	error = device_add_attrs(dev);
	if (error)
		goto AttrsError;
	error = bus_add_device(dev);
	if (error)
		goto BusError;
	error = dpm_sysfs_add(dev);
	if (error)
		goto DPMError;
	device_pm_add(dev);

	if (dev->bus)
		blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
					     BUS_NOTIFY_ADD_DEVICE, dev);

	kobject_uevent(&dev->kobj, KOBJ_ADD);
	bus_probe_device(dev);
	if (parent)
		klist_add_tail(&dev->p->knode_parent,
			       &parent->p->klist_children);

	if (dev->class) {
		mutex_lock(&dev->class->p->mutex);
		
		klist_add_tail(&dev->knode_class,
			       &dev->class->p->klist_devices);

		
		list_for_each_entry(class_intf,
				    &dev->class->p->interfaces, node)
			if (class_intf->add_dev)
				class_intf->add_dev(dev, class_intf);
		mutex_unlock(&dev->class->p->mutex);
	}
done:
	put_device(dev);
	return error;
 DPMError:
	bus_remove_device(dev);
 BusError:
	device_remove_attrs(dev);
 AttrsError:
	device_remove_class_symlinks(dev);
 SymlinkError:
	if (MAJOR(dev->devt))
		devtmpfs_delete_node(dev);
	if (MAJOR(dev->devt))
		device_remove_sys_dev_entry(dev);
 devtattrError:
	if (MAJOR(dev->devt))
		device_remove_file(dev, &devt_attr);
 ueventattrError:
	device_remove_file(dev, &uevent_attr);
 attrError:
	kobject_uevent(&dev->kobj, KOBJ_REMOVE);
	kobject_del(&dev->kobj);
 Error:
	cleanup_device_parent(dev);
	if (parent)
		put_device(parent);
name_error:
	kfree(dev->p);
	dev->p = NULL;
	goto done;
}

int device_register(struct device *dev)
{
	device_initialize(dev);
	return device_add(dev);
}

struct device *get_device(struct device *dev)
{
	return dev ? to_dev(kobject_get(&dev->kobj)) : NULL;
}

void put_device(struct device *dev)
{
	
	if (dev)
		kobject_put(&dev->kobj);
}

void device_del(struct device *dev)
{
	struct device *parent = dev->parent;
	struct class_interface *class_intf;

	if (dev->bus)
		blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
					     BUS_NOTIFY_DEL_DEVICE, dev);
	device_pm_remove(dev);
	dpm_sysfs_remove(dev);
	if (parent)
		klist_del(&dev->p->knode_parent);
	if (MAJOR(dev->devt)) {
		devtmpfs_delete_node(dev);
		device_remove_sys_dev_entry(dev);
		device_remove_file(dev, &devt_attr);
	}
	if (dev->class) {
		device_remove_class_symlinks(dev);

		mutex_lock(&dev->class->p->mutex);
		
		list_for_each_entry(class_intf,
				    &dev->class->p->interfaces, node)
			if (class_intf->remove_dev)
				class_intf->remove_dev(dev, class_intf);
		
		klist_del(&dev->knode_class);
		mutex_unlock(&dev->class->p->mutex);
	}
	device_remove_file(dev, &uevent_attr);
	device_remove_attrs(dev);
	bus_remove_device(dev);
	driver_deferred_probe_del(dev);

	devres_release_all(dev);

	if (platform_notify_remove)
		platform_notify_remove(dev);
	kobject_uevent(&dev->kobj, KOBJ_REMOVE);
	cleanup_device_parent(dev);
	kobject_del(&dev->kobj);
	put_device(parent);
}

void device_unregister(struct device *dev)
{
	pr_debug("device: '%s': %s\n", dev_name(dev), __func__);
	device_del(dev);
	put_device(dev);
}

static struct device *next_device(struct klist_iter *i)
{
	struct klist_node *n = klist_next(i);
	struct device *dev = NULL;
	struct device_private *p;

	if (n) {
		p = to_device_private_parent(n);
		dev = p->device;
	}
	return dev;
}

const char *device_get_devnode(struct device *dev,
			       umode_t *mode, const char **tmp)
{
	char *s;

	*tmp = NULL;

	
	if (dev->type && dev->type->devnode)
		*tmp = dev->type->devnode(dev, mode);
	if (*tmp)
		return *tmp;

	
	if (dev->class && dev->class->devnode)
		*tmp = dev->class->devnode(dev, mode);
	if (*tmp)
		return *tmp;

	
	if (strchr(dev_name(dev), '!') == NULL)
		return dev_name(dev);

	
	*tmp = kstrdup(dev_name(dev), GFP_KERNEL);
	if (!*tmp)
		return NULL;
	while ((s = strchr(*tmp, '!')))
		s[0] = '/';
	return *tmp;
}

int device_for_each_child(struct device *parent, void *data,
			  int (*fn)(struct device *dev, void *data))
{
	struct klist_iter i;
	struct device *child;
	int error = 0;

	if (!parent->p)
		return 0;

	klist_iter_init(&parent->p->klist_children, &i);
	while ((child = next_device(&i)) && !error)
		error = fn(child, data);
	klist_iter_exit(&i);
	return error;
}

struct device *device_find_child(struct device *parent, void *data,
				 int (*match)(struct device *dev, void *data))
{
	struct klist_iter i;
	struct device *child;

	if (!parent)
		return NULL;

	klist_iter_init(&parent->p->klist_children, &i);
	while ((child = next_device(&i)))
		if (match(child, data) && get_device(child))
			break;
	klist_iter_exit(&i);
	return child;
}

int __init devices_init(void)
{
	devices_kset = kset_create_and_add("devices", &device_uevent_ops, NULL);
	if (!devices_kset)
		return -ENOMEM;
	dev_kobj = kobject_create_and_add("dev", NULL);
	if (!dev_kobj)
		goto dev_kobj_err;
	sysfs_dev_block_kobj = kobject_create_and_add("block", dev_kobj);
	if (!sysfs_dev_block_kobj)
		goto block_kobj_err;
	sysfs_dev_char_kobj = kobject_create_and_add("char", dev_kobj);
	if (!sysfs_dev_char_kobj)
		goto char_kobj_err;

	return 0;

 char_kobj_err:
	kobject_put(sysfs_dev_block_kobj);
 block_kobj_err:
	kobject_put(dev_kobj);
 dev_kobj_err:
	kset_unregister(devices_kset);
	return -ENOMEM;
}

EXPORT_SYMBOL_GPL(device_for_each_child);
EXPORT_SYMBOL_GPL(device_find_child);

EXPORT_SYMBOL_GPL(device_initialize);
EXPORT_SYMBOL_GPL(device_add);
EXPORT_SYMBOL_GPL(device_register);

EXPORT_SYMBOL_GPL(device_del);
EXPORT_SYMBOL_GPL(device_unregister);
EXPORT_SYMBOL_GPL(get_device);
EXPORT_SYMBOL_GPL(put_device);

EXPORT_SYMBOL_GPL(device_create_file);
EXPORT_SYMBOL_GPL(device_remove_file);

struct root_device {
	struct device dev;
	struct module *owner;
};

inline struct root_device *to_root_device(struct device *d)
{
	return container_of(d, struct root_device, dev);
}

static void root_device_release(struct device *dev)
{
	kfree(to_root_device(dev));
}

struct device *__root_device_register(const char *name, struct module *owner)
{
	struct root_device *root;
	int err = -ENOMEM;

	root = kzalloc(sizeof(struct root_device), GFP_KERNEL);
	if (!root)
		return ERR_PTR(err);

	err = dev_set_name(&root->dev, "%s", name);
	if (err) {
		kfree(root);
		return ERR_PTR(err);
	}

	root->dev.release = root_device_release;

	err = device_register(&root->dev);
	if (err) {
		put_device(&root->dev);
		return ERR_PTR(err);
	}

#ifdef CONFIG_MODULES	
	if (owner) {
		struct module_kobject *mk = &owner->mkobj;

		err = sysfs_create_link(&root->dev.kobj, &mk->kobj, "module");
		if (err) {
			device_unregister(&root->dev);
			return ERR_PTR(err);
		}
		root->owner = owner;
	}
#endif

	return &root->dev;
}
EXPORT_SYMBOL_GPL(__root_device_register);

void root_device_unregister(struct device *dev)
{
	struct root_device *root = to_root_device(dev);

	if (root->owner)
		sysfs_remove_link(&root->dev.kobj, "module");

	device_unregister(dev);
}
EXPORT_SYMBOL_GPL(root_device_unregister);


static void device_create_release(struct device *dev)
{
	pr_debug("device: '%s': %s\n", dev_name(dev), __func__);
	kfree(dev);
}

struct device *device_create_vargs(struct class *class, struct device *parent,
				   dev_t devt, void *drvdata, const char *fmt,
				   va_list args)
{
	struct device *dev = NULL;
	int retval = -ENODEV;

	if (class == NULL || IS_ERR(class))
		goto error;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		retval = -ENOMEM;
		goto error;
	}

	dev->devt = devt;
	dev->class = class;
	dev->parent = parent;
	dev->release = device_create_release;
	dev_set_drvdata(dev, drvdata);

	retval = kobject_set_name_vargs(&dev->kobj, fmt, args);
	if (retval)
		goto error;

	retval = device_register(dev);
	if (retval)
		goto error;

	return dev;

error:
	put_device(dev);
	return ERR_PTR(retval);
}
EXPORT_SYMBOL_GPL(device_create_vargs);

struct device *device_create(struct class *class, struct device *parent,
			     dev_t devt, void *drvdata, const char *fmt, ...)
{
	va_list vargs;
	struct device *dev;

	va_start(vargs, fmt);
	dev = device_create_vargs(class, parent, devt, drvdata, fmt, vargs);
	va_end(vargs);
	return dev;
}
EXPORT_SYMBOL_GPL(device_create);

static int __match_devt(struct device *dev, void *data)
{
	dev_t *devt = data;

	return dev->devt == *devt;
}

void device_destroy(struct class *class, dev_t devt)
{
	struct device *dev;

	dev = class_find_device(class, NULL, &devt, __match_devt);
	if (dev) {
		put_device(dev);
		device_unregister(dev);
	}
}
EXPORT_SYMBOL_GPL(device_destroy);

int device_rename(struct device *dev, const char *new_name)
{
	char *old_class_name = NULL;
	char *new_class_name = NULL;
	char *old_device_name = NULL;
	int error;

	dev = get_device(dev);
	if (!dev)
		return -EINVAL;

	pr_debug("device: '%s': %s: renaming to '%s'\n", dev_name(dev),
		 __func__, new_name);

	old_device_name = kstrdup(dev_name(dev), GFP_KERNEL);
	if (!old_device_name) {
		error = -ENOMEM;
		goto out;
	}

	if (dev->class) {
		error = sysfs_rename_link(&dev->class->p->subsys.kobj,
			&dev->kobj, old_device_name, new_name);
		if (error)
			goto out;
	}

	error = kobject_rename(&dev->kobj, new_name);
	if (error)
		goto out;

out:
	put_device(dev);

	kfree(new_class_name);
	kfree(old_class_name);
	kfree(old_device_name);

	return error;
}
EXPORT_SYMBOL_GPL(device_rename);

static int device_move_class_links(struct device *dev,
				   struct device *old_parent,
				   struct device *new_parent)
{
	int error = 0;

	if (old_parent)
		sysfs_remove_link(&dev->kobj, "device");
	if (new_parent)
		error = sysfs_create_link(&dev->kobj, &new_parent->kobj,
					  "device");
	return error;
}

int device_move(struct device *dev, struct device *new_parent,
		enum dpm_order dpm_order)
{
	int error;
	struct device *old_parent;
	struct kobject *new_parent_kobj;

	dev = get_device(dev);
	if (!dev)
		return -EINVAL;

	device_pm_lock();
	new_parent = get_device(new_parent);
	new_parent_kobj = get_device_parent(dev, new_parent);

	pr_debug("device: '%s': %s: moving to '%s'\n", dev_name(dev),
		 __func__, new_parent ? dev_name(new_parent) : "<NULL>");
	error = kobject_move(&dev->kobj, new_parent_kobj);
	if (error) {
		cleanup_glue_dir(dev, new_parent_kobj);
		put_device(new_parent);
		goto out;
	}
	old_parent = dev->parent;
	dev->parent = new_parent;
	if (old_parent)
		klist_remove(&dev->p->knode_parent);
	if (new_parent) {
		klist_add_tail(&dev->p->knode_parent,
			       &new_parent->p->klist_children);
		set_dev_node(dev, dev_to_node(new_parent));
	}

	if (!dev->class)
		goto out_put;
	error = device_move_class_links(dev, old_parent, new_parent);
	if (error) {
		
		device_move_class_links(dev, new_parent, old_parent);
		if (!kobject_move(&dev->kobj, &old_parent->kobj)) {
			if (new_parent)
				klist_remove(&dev->p->knode_parent);
			dev->parent = old_parent;
			if (old_parent) {
				klist_add_tail(&dev->p->knode_parent,
					       &old_parent->p->klist_children);
				set_dev_node(dev, dev_to_node(old_parent));
			}
		}
		cleanup_glue_dir(dev, new_parent_kobj);
		put_device(new_parent);
		goto out;
	}
	switch (dpm_order) {
	case DPM_ORDER_NONE:
		break;
	case DPM_ORDER_DEV_AFTER_PARENT:
		device_pm_move_after(dev, new_parent);
		break;
	case DPM_ORDER_PARENT_BEFORE_DEV:
		device_pm_move_before(new_parent, dev);
		break;
	case DPM_ORDER_DEV_LAST:
		device_pm_move_last(dev);
		break;
	}
out_put:
	put_device(old_parent);
out:
	device_pm_unlock();
	put_device(dev);
	return error;
}
EXPORT_SYMBOL_GPL(device_move);

void device_shutdown(void)
{
	struct device *dev;

	spin_lock(&devices_kset->list_lock);
	while (!list_empty(&devices_kset->list)) {
		dev = list_entry(devices_kset->list.prev, struct device,
				kobj.entry);
		get_device(dev);
		list_del_init(&dev->kobj.entry);
		spin_unlock(&devices_kset->list_lock);

		
		pm_runtime_get_noresume(dev);
		pm_runtime_barrier(dev);

		if (dev->bus && dev->bus->shutdown) {
			dev_dbg(dev, "shutdown\n");
			dev->bus->shutdown(dev);
		} else if (dev->driver && dev->driver->shutdown) {
			dev_dbg(dev, "shutdown\n");
			dev->driver->shutdown(dev);
		}
		put_device(dev);

		spin_lock(&devices_kset->list_lock);
	}
	spin_unlock(&devices_kset->list_lock);
	async_synchronize_full();
}


#ifdef CONFIG_PRINTK

int __dev_printk(const char *level, const struct device *dev,
		 struct va_format *vaf)
{
	if (!dev)
		return printk("%s(NULL device *): %pV", level, vaf);

	return printk("%s%s %s: %pV",
		      level, dev_driver_string(dev), dev_name(dev), vaf);
}
EXPORT_SYMBOL(__dev_printk);

int dev_printk(const char *level, const struct device *dev,
	       const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	int r;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	r = __dev_printk(level, dev, &vaf);
	va_end(args);

	return r;
}
EXPORT_SYMBOL(dev_printk);

#define define_dev_printk_level(func, kern_level)		\
int func(const struct device *dev, const char *fmt, ...)	\
{								\
	struct va_format vaf;					\
	va_list args;						\
	int r;							\
								\
	va_start(args, fmt);					\
								\
	vaf.fmt = fmt;						\
	vaf.va = &args;						\
								\
	r = __dev_printk(kern_level, dev, &vaf);		\
	va_end(args);						\
								\
	return r;						\
}								\
EXPORT_SYMBOL(func);

define_dev_printk_level(dev_emerg, KERN_EMERG);
define_dev_printk_level(dev_alert, KERN_ALERT);
define_dev_printk_level(dev_crit, KERN_CRIT);
define_dev_printk_level(dev_err, KERN_ERR);
define_dev_printk_level(dev_warn, KERN_WARNING);
define_dev_printk_level(dev_notice, KERN_NOTICE);
define_dev_printk_level(_dev_info, KERN_INFO);

#endif
