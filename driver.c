#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/export.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dileep");
MODULE_DESCRIPTION("DUMMY DEVICE");

#define IRQ_NUM 12
#define FIRST_MINOR 10
#define NR_DEVS 2

struct task_struct *task;
spinlock_t lock;

static struct workqueue_struct *workqueue;
static struct work_struct *work;

int myOpen(struct inode *inode, struct file *filep)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

int myRelease(struct inode *inode, struct file *filep)
{
    printk("%s\n", __FUNCTION__);
    return 0;
}

long myioctl(struct file *fp, unsigned int pid, unsigned long num)
{
    struct module *ptr = THIS_MODULE;
    struct task_struct *my_task = current;
    
    printk("%s\n", __FUNCTION__);
    printk(" pid = %d\n comm = %s\n module = %s\n", my_task->pid, my_task->comm, ptr->name);

    return 0;
}

static ssize_t myRead(struct file *filep, char __user *buf, size_t len, loff_t *off)
{
    const char msg[] = "hi from myDev\n";
    printk("%s\n", __FUNCTION__);
    copy_to_user(buf, msg, sizeof(msg));
    return sizeof(msg);
}

static ssize_t myWrite(struct file *filep, const char __user *buf, size_t len, loff_t *off)
{
    char msg[512];
    copy_from_user(msg, buf, len);
    printk("%s msg = %s\n", __FUNCTION__, msg);
    return len;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = myOpen,
    .read = myRead,
    .write = myWrite,
    .release = myRelease,
    .unlocked_ioctl = myioctl};

const char *devname = "myDev";
static dev_t mydev;
static struct cdev *my_cdev;
static struct class *mychar_class;
static struct device *mychar_device;

static void wq_function(struct work_struct *work)
{
    spin_lock(&lock);
    printk("%s\n", __FUNCTION__);
    spin_unlock(&lock);
}

static irqreturn_t dev_irq_handler(int irq, void *dev_id)
{
    printk("%s %d\n", __FUNCTION__, irq);

    spin_lock(&lock);
    work = (struct work_struct *)kmalloc(sizeof(struct work_struct), GFP_KERNEL);
    INIT_WORK(work, wq_function);
    queue_work(workqueue, work);
    spin_unlock(&lock);

    return IRQ_RETVAL(1);
}

int dev_thread(void *data)
{
    while (1)
    {
        printk("%s loop\n", __FUNCTION__);
        msleep(1000);
        if (kthread_should_stop())
            break;
    }
    return 0;
}

static int __init dev_init(void)
{
    int ret = -ENODEV;
    int status;

    printk("%s\n", __FUNCTION__);

    /* threads */
    // task = kthread_run(dev_thread, NULL, "dev_thread init");

    /* worker threads */
    // ret = request_irq(IRQ_NUM, dev_irq_handler, IRQF_SHARED, "dev_some_device", dev_init);
    // if (ret < 0)
    // {
    //     printk("request_irq :: error : %d", ret);
    //     return -1;
    // }

    // spin_lock_init(&lock);
    // workqueue = create_workqueue("dev_workqueue");

    /* add device node */

    /* create major / minor number */
    status = alloc_chrdev_region(&mydev, FIRST_MINOR, NR_DEVS, devname);
    if (status < 0)
    {
        printk("alloc_chrdev_region :: failed :: %d\n", status);
        goto err_alloc_chrdev_region;
    }
    printk("alloc_chrdev_region :: Major :: %d\n", MAJOR(mydev));
    printk("alloc_chrdev_region :: Minor :: %d\n", MINOR(mydev));

    my_cdev = cdev_alloc();
    if (my_cdev == NULL)
    {
        printk("cdev_alloc :: failed \n");
        goto err_cdev_alloc;
    }

    /* register file operations */
    cdev_init(my_cdev, &fops);
    my_cdev->owner = THIS_MODULE;

    /* add device to list */
    status = cdev_add(my_cdev, mydev, NR_DEVS);
    if (status < 0)
    {
        printk("cdev_add :: failed :: %d\n", status);
        goto err_cdev_add;
    }

    /* create entry in sysfs */
    mychar_class = class_create(THIS_MODULE, devname);
    if (IS_ERR(mychar_class))
    {
        printk("class_create :: failed \n");
        goto err_class_create;
    }

    /* add device to /dev */
    mychar_device = device_create(mychar_class, NULL, mydev, NULL, devname);
    if (IS_ERR(mychar_device))
    {
        printk("device_create :: failed \n");
        goto err_device_create;
    }

    return 0;

err_device_create:
    class_destroy(mychar_class);
err_class_create:
    cdev_del(my_cdev);
err_cdev_add:
    kfree(my_cdev);
err_cdev_alloc:
    unregister_chrdev_region(mydev, NR_DEVS);
err_alloc_chrdev_region:
    return ret;
}

static void __exit dev_exit(void)
{
    printk("%s\n", __FUNCTION__);
    // kthread_stop(task);
    // free_irq(IRQ_NUM, dev_init);
    // flush_workqueue(workqueue);
    // destroy_workqueue(workqueue);

    device_destroy(mychar_class, mydev);
    class_destroy(mychar_class);
    cdev_del(my_cdev);
    unregister_chrdev_region(mydev, NR_DEVS);

    return;
}

module_init(dev_init);
module_exit(dev_exit);
