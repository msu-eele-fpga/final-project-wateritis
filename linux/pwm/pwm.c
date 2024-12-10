#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/kstrtox.h>
#define SPAN 16

#define Red_out_OFFSET 0
#define Green_out_OFFSET 4
#define Blue_out_OFFSET 8

   


    /**
    * struct pwm_dev - Private pwm patterns device struct.
    * @base_addr: Pointer to the component's base address
    * @red_out: Address of the red_out register
    * @green_out: Address of the green_out register
    * @blue_out: Address of the blue_out register
    *
    * An pwm_dev struct gets created for each pwm patterns component.
    */
    struct pwm_dev {
    void __iomem *base_addr;
    void __iomem *red_out;
    void __iomem *green_out;
    void __iomem *blue_out;
    void __iomem *peri;
    struct miscdevice miscdev;
    struct mutex lock;
    };

    /**
    *pwm_read() - Read method for the pwm char device
    *@file: pointer to the char device file struct.
    *@buf: User-space buffer to read the value into
    *@count: the number of bytes being requested
    *@offset: the byte offset in the file being read from
    *
    *Return: On success, the number of bytes written is returned and the 
    *offset @offset is advanced by this number. On error, a negative error
    *value is returned
    *
    */
    static ssize_t pwm_read(struct file *file, char __user *buf, size_t count, loff_t *offset){
        size_t ret;
        u32 val;

        struct pwm_dev *priv = container_of(file->private_data, struct pwm_dev, miscdev);

        //Check file offset to make sure we are reading from a valid location.
        if(*offset <0){
            //We can't read from a negative position.
            return -EINVAL;
        }
        if(*offset >= SPAN){
            //We can't read from a position past the end of our device.
            return 0;
        }
        if((*offset % 0x4) != 0){
            // Prevent unaligned access.
            pr_warn("pwm_read: unaligned access\n");
            return -EFAULT;
        }

        val = ioread32(priv->base_addr + *offset);

        //Copy the value to userspace
        ret = copy_to_user(buf, &val, sizeof(val));
        if(ret == sizeof(val)){
            pr_warn("pwm_read: Nothing copied\n");
            return -EFAULT;
        }
        // Increment the file offset by the number of bytes we read
        *offset = *offset + sizeof(val);

        return sizeof(val);
    }

    /**
    * pwm_write() - Write method for the pwm char device
    * @file: Pointer to the char device file struct.
    * @buf: User-space buffer to read the value from.
    * @count: The number of bytes being written.
    * @offset: The byte offset in the file being written to.
    *
    * Return: On success, the number of bytes written is returned and the
    * offset @offset is advanced by this number. On error, a negative error
    * value is returned.
    */
    static ssize_t pwm_write(struct file *file, const char __user *buf,
    size_t count, loff_t *offset)
    {
    size_t ret;
    u32 val;

    struct pwm_dev *priv = container_of(file->private_data,
    struct pwm_dev, miscdev);

    if (*offset < 0) {
    return -EINVAL;
    }
    if (*offset >= SPAN) {
    return 0;
    }
    if ((*offset % 0x4) != 0) {
    pr_warn("pwm_write: unaligned access\n");
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
    pr_warn("pwm_write: nothing copied from user space\n");
    ret = -EFAULT;
    }

    mutex_unlock(&priv->lock);
    return ret;
    }

    // listings 15 - 19

 


    /**
    * blue_out_show() - Return the blue_out value to user-space via sysfs.
    * @dev: Device structure for the pwm component. This
    * device struct is embedded in the pwm' platform
    * device struct.
    * @attr: Unused.
    * @buf: Buffer that gets returned to user-space.
    *
    * Return: The number of bytes read.
    */
    static ssize_t blue_out_show(struct device *dev,
    struct device_attribute *attr, char *buf)
    {
    u8 blue_out;
    struct pwm_dev *priv = dev_get_drvdata(dev);

    blue_out = ioread32(priv->blue_out);

    return scnprintf(buf, PAGE_SIZE, "%u\n", blue_out);
    }

    /**
    * blue_out_store() - Store the blue_out value.
    * @dev: Device structure for the pwm component. This
    * device struct is embedded in the pwm' platform
    * device struct.
    * @attr: Unused.
    * @buf: Buffer that contains the blue_out value being written.
    * @size: The number of bytes being written.
    *
    * Return: The number of bytes stored.
    */
    static ssize_t blue_out_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t size)
    {
    u8 blue_out;
    int ret;
    struct pwm_dev *priv = dev_get_drvdata(dev);

    // Parse the string we received as a u8
    // See https://elixir.bootlin.com/linux/latest/source/lib/kstrtox.c#L289
    ret = kstrtou8(buf, 0, &blue_out);
    if (ret < 0) {
    return ret;
    }

    iowrite32(blue_out, priv->blue_out);

    // Write was successful, so we return the number of bytes we wrote.
    return size;
    }

    /**
    * red_out_show() - Return the red_out value
    * to user-space via sysfs.
    * @dev: Device structure for the pwm component. This
    * device struct is embedded in the pwm' device struct.
    * @attr: Unused.
    * @buf: Buffer that gets returned to user-space.
    *
    * Return: The number of bytes read.
    */
    static ssize_t red_out_show(struct device *dev,
    struct device_attribute *attr, char *buf)
    {
    bool hps_control;

    // Get the private pwm data out of the dev struct
    struct pwm_dev *priv = dev_get_drvdata(dev);

    hps_control = ioread32(priv->red_out);

    return scnprintf(buf, PAGE_SIZE, "%u\n", hps_control);
    }

    /**
    * red_out_store() - Store the red_out value.
    * @dev: Device structure for the pwm component. This
    * device struct is embedded in the pwm'
    * platform device struct.
    * @attr: Unused.
    * @buf: Buffer that contains the red_out value being written.
    * @size: The number of bytes being written.
    *
    * Return: The number of bytes stored.
    */
    static ssize_t red_out_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t size)
    {
        bool hps_control;
        int ret;
        struct pwm_dev *priv = dev_get_drvdata(dev);

        // Parse the string we received as a bool
        // See https://elixir.bootlin.com/linux/latest/source/lib/kstrtox.c#L289
        ret = kstrtobool(buf, &hps_control);
        if (ret < 0) {
        // kstrtobool returned an error
        return ret;
        }

        iowrite32(hps_control, priv->red_out);

        // Write was successful, so we return the number of bytes we wrote.
        return size;
    }


    /**
    * green_out_show() - Return the green_out value to user-space via sysfs.
    * @dev: Device structure for the pwm component. This
    * device struct is embedded in the pwm' platform
    * device struct.
    * @attr: Unused.
    * @buf: Buffer that gets returned to user-space.
    *
    * Return: The number of bytes read.
    */
    static ssize_t green_out_show(struct device *dev,
    struct device_attribute *attr, char *buf)
    {
    u8 green_out;
    struct pwm_dev *priv = dev_get_drvdata(dev);

    green_out = ioread32(priv->green_out);

    return scnprintf(buf, PAGE_SIZE, "%u\n", green_out);
    }

    /**
    * green_out_store() - Store the green_out value.
    * @dev: Device structure for the pwm component. This
    * device struct is embedded in the pwm' platform
    * device struct.
    * @attr: Unused.
    * @buf: Buffer that contains the green_out value being written.
    * @size: The number of bytes being written.
    *
    * Return: The number of bytes stored.
    */
    static ssize_t green_out_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t size)
    {
    u8 green_out;
    int ret;
    struct pwm_dev *priv = dev_get_drvdata(dev);

    // Parse the string we received as a u8
    // See https://elixir.bootlin.com/linux/latest/source/lib/kstrtox.c#L289
    ret = kstrtou8(buf, 0, &green_out);
    if (ret < 0) {
    // kstrtou8 returned an error
    return ret;
    }

    iowrite32(green_out, priv->green_out);

    // Write was successful, so we return the number of bytes we wrote.
    return size;
    }

    // Define sysfs attributes
    static DEVICE_ATTR_RW(red_out);
    static DEVICE_ATTR_RW(green_out);
    static DEVICE_ATTR_RW(blue_out);

    // Create an attribute group so the device core can
    // export the attributes for us.
    static struct attribute *pwm_attrs[] = {
        &dev_attr_red_out.attr,
        &dev_attr_green_out.attr,
        &dev_attr_blue_out.attr,
        NULL,
    };
    ATTRIBUTE_GROUPS(pwm);

    // end of listings 15 - 19




    /**
    *   pwm_fops - FIle operations supported by the
    *                       pwm driver
    *   0owner: the pwm driver owns the file operations; this
    *           ensures that the driver can't be removed while the 
    *           character device is still in use
    *
    *   0READ: The read function
    *   0write: the wrtie function
    *   0llseek: We use the kernel's default_llseek() function; this allows
    *            users to change what position they are writing/reading to/from
    *
    */
    static const struct file_operations pwm_fops = {
        .owner = THIS_MODULE,
        .read = pwm_read,
        .write = pwm_write,
        .llseek = default_llseek,
    };

    /**
    * pwm_probe() - Initialize device when a match is found
    * @pdev: Platform device structure associated with our pwm patterns device;
    * pdev is automatically created by the driver core based upon our
    * pwm patterns device tree node.
    *
    * When a device that is compatible with this pwm patterns driver is found, the
    * driver's probe function is calpwm. This probe function gets calpwm by the
    * kernel when an pwm device is found in the device tree.
    */
    static int pwm_probe(struct platform_device *pdev)
    {
    struct pwm_dev *priv;
    size_t ret;
    /*
    * Allocate kernel memory for the pwm patterns device and set it to 0.
    * GFP_KERNEL specifies that we are allocating normal kernel RAM;
    * see the kmalloc documentation for more info. The allocated memory
    * is automatically freed when the device is removed.
    */
    priv = devm_kzalloc(&pdev->dev, sizeof(struct pwm_dev),
    GFP_KERNEL);
    if (!priv) {
        pr_err("Faipwm to allocate memory\n");
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
        pr_err("Faipwm to request/remap platform device resource\n");
        return PTR_ERR(priv->base_addr);
    }

    // Set the memory addresses for each register.
    priv->red_out = priv->base_addr + Red_out_OFFSET;
    priv->green_out = priv->base_addr + Green_out_OFFSET;
    priv->blue_out = priv->base_addr + Blue_out_OFFSET;

    // Enable software-control mode and turn all the pwms on, just for fun.
    
    iowrite32(0xff, priv->blue_out);

    //Initialize the misc device parameters
    priv->miscdev.minor = MISC_DYNAMIC_MINOR;
    priv->miscdev.name = "pwm";
    priv->miscdev.fops = &pwm_fops;
    priv->miscdev.parent = &pdev->dev;

    iowrite32(1, priv->red_out);
    //Register the misc device; this creates a char dev at /dev/pwm
    ret = misc_register(&priv->miscdev);
    if(ret){
        pr_err("pwm to register misc device");
        return ret;
    }

    /* Attach the pwm patterns's private data to the platform device's struct.
    * This is so we can access our state container in the other functions.
    */
     platform_set_drvdata(pdev, priv);

    pr_info("pwm_probe successful\n"); 
    return 0;
    }

    static int pwm_remove(struct platform_device *pdev)
    {
    // Get the pwm patterns's private data from the platform device.
    struct pwm_dev *priv = platform_get_drvdata(pdev);

    // Disable software-control mode, just for kicks.
    iowrite32(0, priv->red_out);

    //Deregister the misc device and remvoe the /dev/pwm file
    misc_deregister(&priv->miscdev);

    pr_info("pwm_remove successful\n");

    return 0;
    }


    /*
    * Define the compatible property used for matching devices to this driver,
    * then add our device id structure to the kernel's device table. For a device
    * to be matched with this driver, its device tree node must use the same
    * compatible string as defined here.
    */
    static const struct of_device_id pwm_of_match[] = {
    { .compatible = "Vincent,pwm", },
    { }
    };
    MODULE_DEVICE_TABLE(of, pwm_of_match);

    /*
    * struct pwm_driver - Platform driver struct for the pwm driver
    * @probe: Function that's calpwm when a device is found
    * @remove: Function that's calpwm when a device is removed
    * @driver.owner: Which module owns this driver
    * @driver.name: Name of the pwm driver
    * @driver.of_match_table: Device tree match table
    */
    static struct platform_driver pwm_driver = {
        .probe = pwm_probe,
        .remove = pwm_remove,
        .driver = {
            .owner = THIS_MODULE,
            .name = "pwm",
            .of_match_table = pwm_of_match,
            .dev_groups = pwm_groups,
        },
    };

    /*
    * We don't need to do anything special in module init/exit.
    * This macro automatically handles module init/exit.
    */
    module_platform_driver(pwm_driver);

    MODULE_LICENSE("Dual MIT/GPL");
    MODULE_AUTHOR("Kenneth Vincent");
    MODULE_DESCRIPTION("pwm driver");
