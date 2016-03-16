/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file   men_z149_dpram.c
 *
 *        \author david.robinson@men.de
 *
 *        \brief  DPRAM driver for FPGAs containing a 16z149 unit.
 *
 *     Switches:
 */
/*-------------------------------[ History ]---------------------------------
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2016 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <MEN/men_chameleon.h>
#include <MEN/men_z149_dpram.h>

/* debug helpers */
#ifdef 	DBG
#define DPRINTK(x...)     printk(x)
#define DBG_FCTNNAME   	  printk("%s()\n", __FUNCTION__ )
#else
#define DPRINTK(x...)
#define DBG_FCTNNAME
#endif

#define MEN_DPRAM_NAME    "13z149_dpram"
#define MEN_DPRAM_DEVICE  "dpram"
#define MEN_DPRAM_COUNT   1
#define MEN_DPRAM_MINOR   1 /* prevent conflict with mdis device node */

struct MEN_13Z149_DPRAM
{
	void *sram_addr;
	u32  sram_size;

	void __iomem *mcr_ioaddr;

	struct pci_dev *pdev;
};

static dev_t dpram_dev;
static struct cdev dpram_cdev;
static struct class *dpram_class;
static struct MEN_13Z149_DPRAM *dpram;

static int men_13z149_open(struct inode *i, struct file *f)
{
	return 0;
}

static int men_13z149_close(struct inode *i, struct file *f)
{
	return 0;
}

static long men_13z149_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct dpram_info info;
	int reg;

	switch (cmd) {
	case DPRAM_IOC_GET_INFO:
		info.address = (unsigned long)dpram->sram_addr;
		info.size = dpram->sram_size;
		if (copy_to_user((struct dpram_info *)arg, &info,
					sizeof(struct dpram_info)))
			return -EFAULT;
		break;

	case DPRAM_IOC_GET_INT_FROM_NIOS:
		reg = ioread8(dpram->mcr_ioaddr + 4) & 0x01;
		if (copy_to_user((int *)arg, &reg, sizeof(int)))
			return -EFAULT;
		break;

	case DPRAM_IOC_RESET_INT_FROM_NIOS:
		reg = ioread8(dpram->mcr_ioaddr) & 0x01;
		if (reg == DPRAM_INT_MODE_FROM_NIOS_INTERRUPT)
			return -EINVAL;
		iowrite8(0x01, dpram->mcr_ioaddr + 4);
		break;

	case DPRAM_IOC_SET_INT_TO_NIOS:
		reg = ioread8(dpram->mcr_ioaddr);
		reg |= 0x02;
		iowrite8(reg, dpram->mcr_ioaddr);
		break;
#if 0
	case DPRAM_IOC_GET_INT_MODE_FROM_NIOS:
		reg = ioread8(dpram->mcr_ioaddr) & 0x01;
		if (copy_to_user((int *)arg, &reg, sizeof(int)))
			return -EFAULT;
		break;

	case DPRAM_IOC_SET_INT_MODE_FROM_NIOS:
		if (copy_from_user(&reg, (int *)arg, sizeof(int)))
			return -EFAULT;
		if (reg != DPRAM_INT_MODE_FROM_NIOS_POLLING &&
		    reg != DPRAM_INT_MODE_FROM_NIOS_INTERRUPT)
			return -EINVAL;
		iowrite8(reg, dpram->mcr_ioaddr);
		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}

static int men_13z149_mmap(struct file *file, struct vm_area_struct *vma)
{
	size_t size = vma->vm_end - vma->vm_start;
	unsigned long pfn = ((unsigned long) dpram->sram_addr) >> PAGE_SHIFT;

	if (size > dpram->sram_size)
		return -EINVAL;

	if (vma->vm_pgoff != pfn)
		return -EINVAL;

	if (remap_pfn_range(vma,
			    vma->vm_start,
			    vma->vm_pgoff, 
			    size,
			    vma->vm_page_prot)) {
		return -EAGAIN;
	}
	
	return 0;
}

static struct file_operations dpram_fops =
{
	.owner		= THIS_MODULE,
	.open		= men_13z149_open,
	.release	= men_13z149_close,
	.unlocked_ioctl	= men_13z149_ioctl,
	.mmap		= men_13z149_mmap,
};

/**********************************************************************/
/** allocate one Element of the linked list and return it.
 *
 * \brief  men_13z149_AllocateDevice allocates all memory
 *         needed to hold one 13Z149 device struct. Clears the struct
 *         to zero.
 *
 * \returns    pointer to the new struct or NULL if error
 */
static struct MEN_13Z149_DPRAM *men_13z149_AllocateDevice(void)
{
	struct MEN_13Z149_DPRAM *newP = NULL;
	
	newP = (struct MEN_13Z149_DPRAM*)(kmalloc(sizeof(struct MEN_13Z149_DPRAM),
	            GFP_KERNEL));

	if (!newP)
		return NULL;
	
	memset(newP, 0, sizeof(struct MEN_13Z149_DPRAM));
	
	return newP;
}

/*******************************************************************/
/** PNP function DPRAM
 *
 * \param chu	\IN data of found unit, passed by chameleon driver
 *
 * \return 0 on success or negative linux error number
 *
 */
static int men_13z149_probe(CHAMELEONV2_UNIT_T *chu)
{
	struct MEN_13Z149_DPRAM *dpramP = NULL;
	CHAMELEONV2_UNIT_T u; 
	int i = 0;
	
	/*---------------------------------+
	| alloc space for one DPRAM device |
	+---------------------------------*/
	dpramP = men_13z149_AllocateDevice();
	if (!dpramP) {
	  	printk(KERN_ERR "*** %s: cant allocate device.\n",
				MEN_DPRAM_NAME);
	  	return -ENOMEM;
	}

	dpramP->pdev = chu->pdev;
	dpramP->mcr_ioaddr = ioremap((phys_addr_t)chu->unitFpga.addr,
				      chu->unitFpga.size);

	/*----------------------+
	| find our SRAM in FPGA |
	+----------------------*/
	do {
		if (men_chameleonV2_unit_find(24, i++, &u)) {
	    		printk(KERN_ERR "*** %s: cant find associated Z024 SRAM.\n",
	    		               MEN_DPRAM_NAME);
			return -ENODEV;
	    	}
	} while (chu->unitFpga.group != u.unitFpga.group
			 || chu->pdev->devfn != u.pdev->devfn
			 || chu->pdev->bus->number != u.pdev->bus->number);
	
	DPRINTK("%s: found CHAMELEON_16Z024_SRAM.\n", MEN_DPRAM_NAME );

	dpramP->sram_addr = u.unitFpga.addr;
	dpramP->sram_size = u.unitFpga.size;

	chu->driver_data = dpramP; /* chu = DPRAM unit here for later remove() */
	dpram = dpramP; /* For usage with file_operations */
	
	return 0;
}



/**********************************************************************/
/** DPRAM driver deregistration from men_chameleon subsystem
 *
 * \param  chu  IN Chameleon unit to deregister
 *
 * \returns Errorcode if error  or 0 on success
 */
static int men_13z149_remove(CHAMELEONV2_UNIT_T *chu)
{
	struct MEN_13Z149_DPRAM *dpramP;

	dpramP = (struct MEN_13Z149_DPRAM *) chu->driver_data;
	if (!dpramP)
		kfree(dpramP);

	return 0;
}


static const u16 G_devIdArr[] = { 149, CHAMELEONV2_DEVID_END };

static CHAMELEONV2_DRIVER_T G_driver =
{
	.name		= MEN_DPRAM_NAME,
	.devIdArr	= G_devIdArr,
	.probe		= men_13z149_probe,
	.remove		= men_13z149_remove,
};


/**********************************************************************/
/** DPRAM driver registration / initialization at men_chameleon
 *  subsystem
 *
 * \returns Errorcode if error  or 0 on success
 */
int __init men_13z149_init(void)
{
	struct device *dev_ret;
	int ret;

	if (!men_chameleonV2_register_driver(&G_driver)) {
		ret = -ENODEV;
		goto out_err;
	}

	ret = alloc_chrdev_region(&dpram_dev, MEN_DPRAM_MINOR, MEN_DPRAM_COUNT,
			MEN_DPRAM_NAME);
	if (ret < 0)
		goto out_err_1;

	cdev_init(&dpram_cdev, &dpram_fops);

	ret = cdev_add(&dpram_cdev, dpram_dev, MEN_DPRAM_COUNT);
	if (ret < 0)	
		goto out_err_2;

	dpram_class = class_create(THIS_MODULE, MEN_DPRAM_NAME);
	if (IS_ERR(dpram_class)) {
		ret = PTR_ERR(dpram_class);
		goto out_err_3;
	}

	dev_ret = device_create(dpram_class, NULL, dpram_dev,
			NULL,MEN_DPRAM_DEVICE);
	if (IS_ERR(dev_ret)) {
		ret = PTR_ERR(dev_ret);
		goto out_err_4;
	}

	return 0;

out_err_4:
	class_destroy(dpram_class);
out_err_3:
	cdev_del(&dpram_cdev);
out_err_2:
	unregister_chrdev_region(dpram_dev, MEN_DPRAM_COUNT);
out_err_1:
	men_chameleonV2_unregister_driver(&G_driver);
out_err:
	return ret;
}

/**********************************************************************/
/** modularized cleanup function
 *
 *
 */
static void __exit men_13z149_cleanup(void)
{
	device_destroy(dpram_class, dpram_dev);
	class_destroy(dpram_class);
	cdev_del(&dpram_cdev);
	unregister_chrdev_region(dpram_dev, MEN_DPRAM_COUNT);

	/* this calls .remove() automatically */
	men_chameleonV2_unregister_driver(&G_driver);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("david.robinson@men.de");
MODULE_DESCRIPTION("MEN 13z149 DPRAM driver");

module_init(men_13z149_init);
module_exit(men_13z149_cleanup);
