/* SPDX-License-Identifier: GPL-2.0 or MIT */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

// ADC channel register addresses
static u32 CH0 = 0x0;
static u32 CH1 = 0x4;
static u32 CH2 = 0x8;
static u32 CH3 = 0xc;
static u32 CH4 = 0x10;
static u32 CH5 = 0x14;
static u32 CH6 = 0x18;
static u32 CH7 = 0x1c;

// register addresses to trigger updates and enable auto-update
#define UPDATE 0x0
#define AUTO_UPDATE 0x4

#define SPAN 32

// ADC values are in the 12 least-significant bits of the registers
#define ADC_VALUE_BITMASK 0xfff

static unsigned long VOLTAGE_SCALE_MV = 1;

/**
 * struct adc_dev - Private led patterns device struct.
 * @base_addr: Pointer to the component's base address 
 * @hps_led_control: Pointer to the hps_led_control register 
 * @base_period: Pointer to the base_period register 
 * @led_reg: Pointer to the led_reg register 
 * @miscdev: miscdevice used to create a character device
 * @lock: mutex used to prevent concurrent writes to memory 
 *
 * An adc_dev struct gets created for each led patterns component.
 */
struct adc_dev {
	void __iomem *base_addr;
	bool auto_update;
	struct miscdevice miscdev;
	struct mutex lock;
};

/**
 * adc_read() - Read method for the adc char device
 * @file: Pointer to the char device file struct.
 * @buf: User-space buffer to read the value into.
 * @count: The number of bytes being requested.
 * @offset: The byte offset in the file being read from.
 *
 * Return: On success, the number of bytes written is returned and the
 * offset @offset is advanced by this number. On error, a negative error
 * value is returned.
 */
static ssize_t adc_read(struct file *file, char __user *buf,
	size_t count, loff_t *offset)
{
	size_t ret;
	u32 val;

	/*
	 * Get the device's private data from the file struct's private_data
	 * field. The private_data field is equal to the miscdev field in the
	 * adc_dev struct. container_of returns the 
     * adc_dev struct that contains the miscdev in private_data.
	 */
	struct adc_dev *priv = container_of(file->private_data,
	                            struct adc_dev, miscdev);

	// Check file offset to make sure we are reading from a valid location.
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
		pr_warn("adc_read: unaligned access\n");
		return -EFAULT;
	}

	val = ioread32(priv->base_addr + *offset) & ADC_VALUE_BITMASK;

	// Copy the value to userspace.
	ret = copy_to_user(buf, &val, sizeof(val));
	if (ret == sizeof(val)) {
		pr_warn("adc_read: nothing copied\n");
		return -EFAULT;
	}

	// Increment the file offset by the number of bytes we read.
	*offset = *offset + sizeof(val);

	return sizeof(val);
}

/**
 * adc_write() - Write method for the adc char device
 * @file: Pointer to the char device file struct.
 * @buf: User-space buffer to read the value from.
 * @count: The number of bytes being written.
 * @offset: The byte offset in the file being written to.
 *
 * Return: On success, the number of bytes written is returned and the
 * offset @offset is advanced by this number. On error, a negative error
 * value is returned.
 */
static ssize_t adc_write(struct file *file, const char __user *buf,
	size_t count, loff_t *offset)
{
	size_t ret;
	u32 val;

	struct adc_dev *priv = container_of(file->private_data,
	                              struct adc_dev, miscdev);

	if (*offset < 0) {
		return -EINVAL;
	}
	if (*offset >= AUTO_UPDATE) {
		// can't write past to the read-only adc channel registers
		return -EINVAL;
	}
	if ((*offset % 0x4) != 0) {
		pr_warn("adc_write: unaligned access\n");
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
	}
	else {
		pr_warn("adc_write: nothing copied from user space\n");
		ret = -EFAULT;
	}

	mutex_unlock(&priv->lock);
	return ret;
}

/** 
 *  adc_fops - File operations supported by the  
 *                          adc driver
 * @owner: The adc driver owns the file operations; this 
 *         ensures that the driver can't be removed while the 
 *         character device is still in use.
 * @read: The read function.
 * @write: The write function.
 * @llseek: We use the kernel's default_llseek() function; this allows 
 *          users to change what position they are writing/reading to/from.
 */
static const struct file_operations  adc_fops = {
	.owner = THIS_MODULE,
	.read = adc_read,
	.write = adc_write,
	.llseek = default_llseek,
};

/**
 * XXX: both update and auto_update appear to be useless. The ADC *always*
 * auto updates regardless of what settings are used. Not that we can tell
 * what settings are used since the registers are read-only... :^| stupid!!!
 * Setting auto_update does set bit 15 high as a "refresh flag" as described
 * in the VHDL component's datasheet, but the values update every time you read
 * a channel register even when auto_update is off and bit 15 is low.
 * (╯°□°）╯︵ ┻━┻
 * Whatever... the adc channels update when we read, so that's all we want...
 */
/**
 * update_store() - Start new ADC conversions.
 * 
 * Writing *any* value to the update register triggers an update.
 * 
 * @dev: Device structure for the adc component. This 
 *       device struct is embedded in the adc's platform 
 *       device struct.
 * @attr: Unused.
 * @buf: Buffer that contains the value being written.
 * @size: The number of bytes being written.
 *
 * Return: The number of bytes stored.
 */
static ssize_t update_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adc_dev *priv = dev_get_drvdata(dev);

	/* 
	 * Since writing any value to the update register triggers an update,
	 * it doesn't matter what we write or what the user writes. So we ignore
	 * what the user wants to write and just write a 1 :)
	 */
	iowrite32(1, priv->base_addr + UPDATE);

	return 4;
}

/**
 * auto_update_store() - Enable/disable auto-update
 * 
 * @dev: Device structure for the adc component. 
 * @attr: Unused.
 * @buf: Buffer that contains the value being written.
 * @size: The number of bytes being written.
 *
 * Return: The number of bytes stored.
 */
static ssize_t auto_update_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{

	int ret;
	struct adc_dev *priv = dev_get_drvdata(dev);

	ret = kstrtobool(buf, &(priv->auto_update));
	if (ret < 0) {
		return ret;
	}

	iowrite32(priv->auto_update, priv->base_addr + AUTO_UPDATE);

	return size;
}

/**
 * auto_update_show() - Read the auto_update setting.
 * @dev: Device structure for the adc component. 
 * @attr: Unused.
 * @buf: Buffer that gets returned to user-space.
 *
 * Return: The number of bytes read.
 */
static ssize_t auto_update_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adc_dev *priv = dev_get_drvdata(dev);

	/*
	 * The auto_update register is actually a write-only register (dumb!), so
	 * we return our shadow copy.
	 */
	return scnprintf(buf, PAGE_SIZE, "%u\n", priv->auto_update);
}

/**
 * adc_ch_show() - Read ADC channel value.
 * 
 * @dev: Device structure for the adc component. 
 * @attr: Which adc channel attribute we're reading from.
 * @buf: Buffer that gets returned to user-space.
 *
 * Return: The number of bytes read.
 */
static ssize_t adc_ch_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 adc_value;
	struct adc_dev *priv = dev_get_drvdata(dev);

	struct dev_ext_attribute *ch_attr = container_of(attr, 
		struct dev_ext_attribute, attr);

	u32 ch_offset = *(u32 *)(ch_attr->var);

	adc_value = ioread32(priv->base_addr + ch_offset) & ADC_VALUE_BITMASK;

	return scnprintf(buf, PAGE_SIZE, "%u\n", adc_value);
}

/*
 * DEVICE_ADC_CH_ATTR uses the dev_ext_attribute struct so we can pass in the
 * channel's offset to the sysfs store function, allowing us to only write one
 * store function instead of 8 identical store functions.
 * https://elixir.bootlin.com/linux/v6.12/source/include/linux/device.h#L118
 * https://stackoverflow.com/questions/48540242/how-can-i-create-lots-of-similar-functions-for-sysfs-attributes
 */
#define DEVICE_ADC_CH_ATTR(_name, _reg_offset) \
	struct dev_ext_attribute dev_attr_##_name = \
		{ __ATTR(_name, 0444, adc_ch_show, NULL), &(_reg_offset) }

#define DEVICE_ULONG_ATTR_RO(_name, _var) \
	struct dev_ext_attribute dev_attr_##_name = \
		{ __ATTR(_name, 0444, device_show_ulong, NULL), &(_var) }

static DEVICE_ATTR_WO(update);
static DEVICE_ATTR_RW(auto_update);
static DEVICE_ADC_CH_ATTR(ch0_raw, CH0);
static DEVICE_ADC_CH_ATTR(ch1_raw, CH1);
static DEVICE_ADC_CH_ATTR(ch2_raw, CH2);
static DEVICE_ADC_CH_ATTR(ch3_raw, CH3);
static DEVICE_ADC_CH_ATTR(ch4_raw, CH4);
static DEVICE_ADC_CH_ATTR(ch5_raw, CH5);
static DEVICE_ADC_CH_ATTR(ch6_raw, CH6);
static DEVICE_ADC_CH_ATTR(ch7_raw, CH7);
static DEVICE_ULONG_ATTR_RO(voltage_scale_mv, VOLTAGE_SCALE_MV);

static struct attribute *adc_attrs[] = {
	&dev_attr_update.attr,
	&dev_attr_auto_update.attr,
	&dev_attr_ch0_raw.attr.attr,
	&dev_attr_ch1_raw.attr.attr,
	&dev_attr_ch2_raw.attr.attr,
	&dev_attr_ch3_raw.attr.attr,
	&dev_attr_ch4_raw.attr.attr,
	&dev_attr_ch5_raw.attr.attr,
	&dev_attr_ch6_raw.attr.attr,
	&dev_attr_ch7_raw.attr.attr,
	&dev_attr_voltage_scale_mv.attr.attr,
	NULL,
};
ATTRIBUTE_GROUPS(adc);

/**
 * adc_probe() - Initialize device when a match is found
 * @pdev: Platform device structure associated with our led patterns device;
 *        pdev is automatically created by the driver core based upon our
 *        led patterns device tree node.
 *
 * When a device that is compatible with this led patterns driver is found, the
 * driver's probe function is called. This probe function gets called by the
 * kernel when an adc device is found in the device tree.
 */
static int adc_probe(struct platform_device *pdev)
{
	struct adc_dev *priv;
	size_t ret;

	/*
	 * Allocate kernel memory for the led patterns device and set it to 0.
	 * GFP_KERNEL specifies that we are allocating normal kernel RAM;
	 * see the kmalloc documentation for more info. The allocated memory
	 * is automatically freed when the device is removed.
	 */
	priv = devm_kzalloc(&pdev->dev, sizeof(struct adc_dev),
	                    GFP_KERNEL);
	if (!priv) {
		pr_err("Failed to allocate memory\n");
		return -ENOMEM;
	}

	/*
	 * Request and remap the device's memory region. Requesting the region
	 * make sure nobody else can use that memory. The memory is remapped
	 * into the kernel's virtual address space because we don't have access
	 * to physical memory locations.
	 */
	priv->base_addr = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->base_addr)) {
		pr_err("Failed to request/remap platform device resource\n");
		return PTR_ERR(priv->base_addr);
	}

	// Initialize the misc device parameters
	priv->miscdev.minor = MISC_DYNAMIC_MINOR;
	priv->miscdev.name = "adc";
	priv->miscdev.fops = &adc_fops;
	priv->miscdev.parent = &pdev->dev;

	// Register the misc device; this creates a char dev at /dev/adc
	ret = misc_register(&priv->miscdev);
	if (ret) {
		pr_err("Failed to register misc device");
		return ret;
	}

	/*
	 * Attach the led patterns's private data to the platform device's struct.
	 * This is so we can access our state container in the other functions.
	 */
	platform_set_drvdata(pdev, priv);

	pr_info("adc_probe successful\n");

	return 0;
}

/**
 * adc_probe() - Remove an led patterns device.
 * @pdev: Platform device structure associated with our led patterns device.
 *
 * This function is called when an led patterns devicee is removed or
 * the driver is removed.
 */
static int adc_remove(struct platform_device *pdev)
{
	// Get the led patterns's private data from the platform device.
	struct adc_dev *priv = platform_get_drvdata(pdev);

	// Deregister the misc device and remove the /dev/adc file.
	misc_deregister(&priv->miscdev);

	pr_info("adc_remove successful\n");

	return 0;
}


/*
 * Define the compatible property used for matching devices to this driver,
 * then add our device id structure to the kernel's device table. For a device
 * to be matched with this driver, its device tree node must use the same
 * compatible string as defined here.
 */
static const struct of_device_id adc_of_match[] = {
	{ .compatible = "adsd,de10nano_adc", },
	{ }
};
MODULE_DEVICE_TABLE(of, adc_of_match);

/**
 * struct adc_driver - Platform driver struct for the adc driver
 * @probe: Function that's called when a device is found
 * @remove: Function that's called when a device is removed
 * @driver.name: Name of the led patterns driver
 * @driver.of_match_table: Device tree match table
 * @driver.dev_groups: sysfs attribute group
 */
static struct platform_driver adc_driver = {
	.probe = adc_probe,
	.remove = adc_remove,
	.driver = {
        .owner = THIS_MODULE,
		.name = "adc",
		.of_match_table = adc_of_match,
		.dev_groups = adc_groups,
	},
};

/*
 * We don't need to do anything special in module init/exit.
 * This macro automatically handles module init/exit.
 */
module_platform_driver(adc_driver);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Trevor Vannoy");
MODULE_DESCRIPTION("adc driver");
MODULE_VERSION("1.0");