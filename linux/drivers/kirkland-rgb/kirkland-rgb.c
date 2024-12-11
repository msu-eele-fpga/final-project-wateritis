#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/io.h> //iowrite32/ioread32 functions
#include <linux/mutex.h> // mutex definitions
#include <linux/miscdevice.h> // miscdevice definitions
#include <linux/types.h> // data types like u32, u16, etc.
#include <linux/fs.h> // copy_to_user, etc
#include <linux/kstrtox.h> // kstrtou8, etc


#define PERIOD_REG_OFFSET 0
#define RED_DUTY_CYCLE_OFFSET 4
#define GRN_DUTY_CYCLE_OFFSET 8
#define BLU_DUTY_CYCLE_OFFSET 12
#define SPAN 16

static struct platform_driver kirkland_rgb_driver;
static const struct of_device_id kirkland_rgb_of_match[];
static const struct file_operations kirkland_rgb_fop;
static int kirkland_rgb_probe(struct platform_device *pdev);
static int kirkland_rgb_remove(struct platform_device *pdev);
static ssize_t kirkland_rgb_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t kirkland_rgb_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

static ssize_t period_reg_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t period_reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
static ssize_t red_duty_cycle_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t red_duty_cycle_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
static ssize_t grn_duty_cycle_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t grn_duty_cycle_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
static ssize_t blu_duty_cycle_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t blu_duty_cycle_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
static struct attribute *kirkland_rgb_attrs[];

// Define sysfs attributes
static DEVICE_ATTR_RW(period_reg);
static DEVICE_ATTR_RW(red_duty_cycle);
static DEVICE_ATTR_RW(grn_duty_cycle);
static DEVICE_ATTR_RW(blu_duty_cycle);

// Create an attribute group so the device core can
// export the attributes for us.
static struct attribute *kirkland_rgb_attrs[] = {
	&dev_attr_period_reg.attr,
	&dev_attr_red_duty_cycle.attr,
	&dev_attr_grn_duty_cycle.attr,
	&dev_attr_blu_duty_cycle.attr,
	NULL,
};
ATTRIBUTE_GROUPS(kirkland_rgb);

/*struct platform_driver {
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	struct device_driver driver;
}; */

/**
 * struct kirkland_rgb_dev - Private led patterns device struct.
 * @base_addr: Pointer to the component's base address
 * @period_reg: Address of the period_reg register
 * @red_duty_cycle: Address of the red_duty_cycle register
 * @grn_duty_cycle: Address of the grn_duty_cycle register
 * @blu_duty_cycle: Address of the blu_duty_cycle register
 *
 * A kirkland_rgb_dev struct gets created for each rgb controller component.
 */
struct kirkland_rgb_dev {
	void __iomem *base_addr;
	void __iomem *period_reg;
	void __iomem *red_duty_cycle;
	void __iomem *grn_duty_cycle;
	void __iomem *blu_duty_cycle;
	struct miscdevice miscdev;
	struct mutex lock;
};

/**
 * struct kirkland_rgb_driver - Platform driver struct for the kirkland_rgb driver
 * @probe: Function that's called when a device is found
 * @remove: Function that's called when a device is removed
 * @driver.owner: Which module owns this driver
 * @driver.name: Name of the kirkland_rgb driver
 * @driver.of_match_table: Device tree match table
 */
static struct platform_driver kirkland_rgb_driver = {
	.probe = kirkland_rgb_probe,
	.remove = kirkland_rgb_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "kirkland_rgb",
		.of_match_table = kirkland_rgb_of_match,
		.dev_groups = kirkland_rgb_groups,
	},
};


/**
 * kirkland_rgb_fops - File operations supported by the kirkland_rgb driver
 *
 * @owner: The kirkland_rgb driver owns the file operations; this
 * ensures that the driver can't be removed while the character
 * device is still in use.
 * @read: The read function.
 * @write: The write function
 * @llseek: We use the kernel's default_llseek() function; this allows
 * users to change what position they are writing/reading to/from.
 */
 static const struct file_operations kirkland_rgb_fops = {
	.owner = THIS_MODULE,
	.read = kirkland_rgb_read,
	.write = kirkland_rgb_write,
	.llseek = default_llseek,
 };

/**
 * kirkland_rgb_probe() - Initialize device when a match is found
 * @pdev: Platform device structure associated with our rgb controller device;
 * 	pdev is automatically created by the driver core based upon our 
 * 	rgb controller device tree node.
 * 
 * When a device that is compatible with this rgb controller driver is found, the
 * driver's probe function is called. This probe function gets called by the
 * kernel when an kirkland_rgb device is found in the device tree. 
 */
static int kirkland_rgb_probe(struct platform_device *pdev) {
	struct kirkland_rgb_dev *priv;
	size_t ret;

	/*
	 * Allocate kernel memory for the led patterns device and set it to 0.
	 * GFP_KERNEL specifies that we are allocating normal kernel RAM;
	 * see the kmalloc documentation for more info. The allocated memory
	 * is automatically freed when the device is removed. 
	 */

	priv = devm_kzalloc(&pdev->dev, sizeof(struct kirkland_rgb_dev), GFP_KERNEL);

	if (!priv) {
		pr_err("Failed to allocate memory\n");
		return -ENOMEM;
	}

	/*
	 * Request and remap the device's memory region. Requesting the region
	 * makes sure nobody else can use that memory. The memory is remapped 
	 * into the kernel's virtual address space because we don't have access
	 * to physical memory locations. 
	 */
	priv->base_addr = devm_platform_ioremap_resource(pdev, 0);
	
	if (IS_ERR(priv->base_addr)) {
		pr_err("Failed to request/remap platform device resource\n");
		return PTR_ERR(priv->base_addr);
	}

	// Set the memory addresses for each register.
	priv->period_reg = priv->base_addr + PERIOD_REG_OFFSET;
	priv->red_duty_cycle = priv->base_addr + RED_DUTY_CYCLE_OFFSET;
	priv->grn_duty_cycle = priv->base_addr + GRN_DUTY_CYCLE_OFFSET;
	priv->blu_duty_cycle = priv->base_addr + BLU_DUTY_CYCLE_OFFSET;

	// Set default register values
	iowrite32(0x80, priv->period_reg);
	iowrite32(0x100000, priv->red_duty_cycle);
	iowrite32(0x80000, priv->grn_duty_cycle);
	iowrite32(0x40000, priv->blu_duty_cycle);

	// Initialize the misc device paramters
	priv->miscdev.minor = MISC_DYNAMIC_MINOR;
	priv->miscdev.name = "kirkland_rgb";
	priv->miscdev.fops = &kirkland_rgb_fops;
	priv->miscdev.parent = &pdev->dev;

	// Register the misc device; this creates a char dev at /dev/kirkland_rgb
	ret = misc_register(&priv->miscdev);
	if (ret) {
		pr_err("Failed to register misc device");
		return ret;
	}

	/* Attach the led pattern's private data to the platform device's struct.
	 * This is so we can access our state container in the other functions.
	 */
	platform_set_drvdata(pdev, priv);

	pr_info("kirkland_rgb_probe successful\n");

	return 0;
}

/**
 * kirkland_rgb_remove() - Remove a rgb controller device.
 * @pdev: Platform device structure associated with our rgb controller device.
 * 
 * This function is called when an rgb controller device is removed or
 * the driver is removed
 */
static int kirkland_rgb_remove(struct platform_device *pdev) {
	// get the led patterns's private data from the platform device.
	struct kirkland_rgb_dev *priv = platform_get_drvdata(pdev);

	// Deregister the misc device and remove the /dev/kirkland_rgb file.
	misc_deregister(&priv->miscdev);

	pr_info("kirkland_rgb_remove successful\n");

	return 0;
}

/**
 * kirkland_rgb_read() - Read method for the kirkland_rgb char device
 * @file: Pointer to the char device file struct.
 * @buf: User-space buffer to read the value into.
 * @count: The number of bytes being requested.
 * @offset: The byte offset in the file being read from.
 * 
 * Return: On success, the number of bytes written is returned and the
 * offset @offset is advanced by this number. On error, a negative error
 * value is returned.
 */
 static ssize_t kirkland_rgb_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
	size_t ret;
	u32 val;

	/* Get the device's private data from the file struct's private_data field.
	 * The private_data field is equal to the miscdev field in the kirkland_rgb_dev
	 * struct. container_of returns the kirkland_rgb_dev struct that contains the 
	 * miscdev in private_data. 
	 */
	 struct kirkland_rgb_dev *priv = container_of(file->private_data, struct kirkland_rgb_dev, miscdev);
	 
	 // Check the file offset to make sure we are reading from a valid location.
	 if (*offset < 0) {
		// We can't read from a negative file position.
		return -EINVAL;
	 }
	 if (*offset >= SPAN) {
		// We can't read from a position past the end of our device.
		return 0;
	 }
	 if ((*offset % 0x4) != 0) {
		// Prevent unaligned access.
		pr_warn("kirkland_rgb_read: unaligned access\n");
		return -EFAULT;
	 }

	 val = ioread32(priv->base_addr + *offset);

	 // Copy the value to userspace.
	 ret = copy_to_user(buf, &val, sizeof(val));
	 if (ret == sizeof(val)) {
		pr_warn("kirkland_rgb_read: nothing copied\n");
		return -EFAULT;
	 }

	 // Increment the file offset by the number of bytes we read.
	 *offset = *offset + sizeof(val);
	 
	 return sizeof(val);
 }

/**
 * kirkland_rgb_write() - Write method for the kirkland_rgb char device
 * @file: Pointer to the char device file struct.
 * @buf: User-space buffer to read the value from.
 * @count: The number of bytes being written.
 * @offset: The byte offset in the file being written to.
 *
 * Return: On success, the number of bytes written is returned and the
 * offset @offset is advanced by this number. On error, a negative error
 * value is returned.
 */
static ssize_t kirkland_rgb_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
	size_t ret;
	u32 val;

	struct kirkland_rgb_dev *priv = container_of(file->private_data, struct kirkland_rgb_dev, miscdev);

	if (*offset < 0) {
		return -EINVAL;
	}
	if (*offset >= SPAN) {
		return 0;
	}
	if ((*offset % 0x4) != 0) {
		pr_warn("kirkland_rgb_write: unaligned access\n");
		return -EFAULT;
	}

	mutex_lock(&priv->lock);

	// Get the value from userspace.
	ret = copy_from_user(&val, buf, sizeof(val));
	if (ret != sizeof(val)) {
		iowrite32(val, priv->base_addr + *offset);

		// Increment the file offset by the number of bytes we wrote.
		*offset = *offset + sizeof(val);

		// Return the number of bytes we wrote.
		ret = sizeof(val);
	} else {
		pr_warn("kirkland_rgb_write: nothing copied from user space\n");
		ret = -EFAULT;
	}

	mutex_unlock(&priv->lock);
	
	return ret;
}

/**
 * period_reg_show() - Return the period_reg value to user-space via sysfs.
 * @dev: Device structure for the kirkland_rgb component. This
 * device struct is embedded in the kirkland_rgb' platform
 * device struct.
 * @attr: Unused.
 * @buf: Buffer that gets returned to user-space.
 *
 * Return: The number of bytes read.
 */
static ssize_t period_reg_show(struct device *dev, struct device_attribute *attr, char *buf) {
	u32 period_reg;
	struct kirkland_rgb_dev *priv = dev_get_drvdata(dev);

	period_reg = ioread32(priv->period_reg);

	return scnprintf(buf, PAGE_SIZE, "%u\n", period_reg);
}

/**
 * period_reg_store() - Store the period_reg value.
 * @dev: Device structure for the kirkland_rgb component. This
 * device struct is embedded in the kirkland_rgb' platform
 * device struct.
 * @attr: Unused.
 * @buf: Buffer that contains the period_reg value being written.
 * @size: The number of bytes being written.
 *
 * Return: The number of bytes stored.
 */
static ssize_t period_reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size) {
	u32 period_reg;
	int ret;
	struct kirkland_rgb_dev *priv = dev_get_drvdata(dev);

	// Parse the string we received as a u8
	// See https://elixir.bootlin.com/linux/latest/source/lib/kstrtox.c#L289
	ret = kstrtou32(buf, 0, &period_reg);
	if (ret < 0) {
		return ret;
	}

	iowrite32(period_reg, priv->period_reg);

	// Write was successful, so we return the number of bytes we wrote.
	return size;
} 

/**
 * red_duty_cycle_show() - Return the red_duty_cycle value
 * to user-space via sysfs.
 * @dev: Device structure for the kirkland_rgb component. This
 * device struct is embedded in the kirkland_rgb' device struct.
 * @attr: Unused.
 * @buf: Buffer that gets returned to user-space.
 *
 * Return: The number of bytes read.
 */
static ssize_t red_duty_cycle_show(struct device *dev, struct device_attribute *attr, char *buf) {
	bool red_duty_cycle;

	// Get the private kirkland_rgb data out of the dev struct
	struct kirkland_rgb_dev *priv = dev_get_drvdata(dev);

	red_duty_cycle = ioread32(priv->red_duty_cycle);

	return scnprintf(buf, PAGE_SIZE, "%u\n", red_duty_cycle);
}

/**
 * red_duty_cycle_store() - Store the red_duty_cycle value.
 * @dev: Device structure for the kirkland_rgb component. This
 * device struct is embedded in the kirkland_rgb'
 * platform device struct.
 * @attr: Unused.
 * @buf: Buffer that contains the red_duty_cycle value being written.
 * @size: The number of bytes being written.
 *
 * Return: The number of bytes stored.
 */
static ssize_t red_duty_cycle_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size) {
	u32 red_duty_cycle;
	int ret;
	struct kirkland_rgb_dev *priv = dev_get_drvdata(dev);

	// Parse the string we received as a bool
	// See https://elixir.bootlin.com/linux/latest/source/lib/kstrtox.c#L289
	ret = kstrtou32(buf, 0, &red_duty_cycle);
	if (ret < 0) {
		// kstrtobool returned an error
		return ret;
	}

	iowrite32(red_duty_cycle, priv->red_duty_cycle);

	// Write was successful, so we return the number of bytes we wrote.
	return size;
}

/**
 * grn_duty_cycle_show() - Return the grn_duty_cycle value to user-space via sysfs.
 * @dev: Device structure for the kirkland_rgb component. This
 * device struct is embedded in the kirkland_rgb' platform
 * device struct.
 * @attr: Unused.
 * @buf: Buffer that gets returned to user-space.
 *
 * Return: The number of bytes read.
 */
static ssize_t grn_duty_cycle_show(struct device *dev, struct device_attribute *attr, char *buf) {
	u32 grn_duty_cycle;
	struct kirkland_rgb_dev *priv = dev_get_drvdata(dev);

	grn_duty_cycle = ioread32(priv->grn_duty_cycle);

	return scnprintf(buf, PAGE_SIZE, "%u\n", grn_duty_cycle);
}

/**
 * grn_duty_cycle_store() - Store the grn_duty_cycle value.
 * @dev: Device structure for the kirkland_rgb component. This
 * device struct is embedded in the kirkland_rgb' platform
 * device struct.
 * @attr: Unused.
 * @buf: Buffer that contains the grn_duty_cycle value being written.
 * @size: The number of bytes being written.
 *
 * Return: The number of bytes stored.
 */
static ssize_t grn_duty_cycle_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size) {
	u32 grn_duty_cycle;
	int ret;
	struct kirkland_rgb_dev *priv = dev_get_drvdata(dev);

	// Parse the string we received as a u8
	// See https://elixir.bootlin.com/linux/latest/source/lib/kstrtox.c#L289
	ret = kstrtou32(buf, 0, &grn_duty_cycle);
	if (ret < 0) {
		// kstrtou8 returned an error
		return ret;
	}

	iowrite32(grn_duty_cycle, priv->grn_duty_cycle);

	// Write was successful, so we return the number of bytes we wrote.
	return size;
}

/**
 * blu_duty_cycle_show() - Return the blu_duty_cycle value to user-space via sysfs.
 * @dev: Device structure for the kirkland_rgb component. This
 * device struct is embedded in the kirkland_rgb' platform
 * device struct.
 * @attr: Unused.
 * @buf: Buffer that gets returned to user-space.
 *
 * Return: The number of bytes read.
 */
static ssize_t blu_duty_cycle_show(struct device *dev, struct device_attribute *attr, char *buf) {
	u32 blu_duty_cycle;
	struct kirkland_rgb_dev *priv = dev_get_drvdata(dev);

	blu_duty_cycle = ioread32(priv->blu_duty_cycle);

	return scnprintf(buf, PAGE_SIZE, "%u\n", blu_duty_cycle);
}

/**
 * blu_duty_cycle_store() - Store the blu_duty_cycle value.
 * @dev: Device structure for the kirkland_rgb component. This
 * device struct is embedded in the kirkland_rgb' platform
 * device struct.
 * @attr: Unused.
 * @buf: Buffer that contains the blu_duty_cycle value being written.
 * @size: The number of bytes being written.
 *
 * Return: The number of bytes stored.
 */
static ssize_t blu_duty_cycle_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size) {
	u32 blu_duty_cycle;
	int ret;
	struct kirkland_rgb_dev *priv = dev_get_drvdata(dev);

	// Parse the string we received as a u8
	// See https://elixir.bootlin.com/linux/latest/source/lib/kstrtox.c#L289
	ret = kstrtou32(buf, 0, &blu_duty_cycle);
	if (ret < 0) {
		// kstrtou8 returned an error
		return ret;
	}

	iowrite32(blu_duty_cycle, priv->blu_duty_cycle);

	// Write was successful, so we return the number of bytes we wrote.
	return size;
}










/**
 * Define the compatible property used for matching devices to this driver,
 * then add our device id structure to the kernel's device table. For a device
 * to be matched with this driver, its device tree node must use the same 
 * compatible string as defined here.
 */
static const struct of_device_id kirkland_rgb_of_match[] = {
	{ .compatible = "Kirkland,kirkland_rgb", },
	{ }	
};

module_platform_driver(kirkland_rgb_driver);
MODULE_DEVICE_TABLE(of, kirkland_rgb_of_match);
MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Grant Kirkland");
MODULE_DESCRIPTION("kirkland_rgb driver");