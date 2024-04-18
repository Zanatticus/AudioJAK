#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <asm/spinlock.h>
#include <linux/uaccess.h>
#include <linux/kfifo.h>
#include <linux/delay.h>
#include <linux/irqflags.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <linux/io.h>

#define DRIVER_NAME "esl-audio"

#define ISR 0x0 // Interrupt Status Register (Read/Clear on Write)
#define IER 0x4 // Interrupt Enable Register (Read/Write)
#define TDFR 0x8 // Transmit Data Fifo Reset (Write)
#define TDFV 0xC // Transmit Data Fifo Vacancy (Read)
#define TDFD 0x10 // Transmit Data FIFO 32-bit Wide Data Write Port (Write)
#define TLR 0x14 // Transmit Length Register (Write)
#define RDFO 0x1C // Receive Data Fifo Occupancy (Read)
#define TDR 0x2C // Transmit Destination Register (Write)

// Interrupt Enable Register Interrupt Bit Numbers/Masks
#define RFPE 1 << 19 // Receive FIFO Programmable Empty
#define RPPF 1 << 20 // Receive FIFO Programmable Full
#define TFPE 1 << 21 // Transmit FIFO Programmable Empty
#define TFPF 1 << 22 // Transmit FIFO Programmable Full
#define RRC 1 << 23 // Receive Reset Complete
#define TRC 1 << 24 // Transmit Reset Complete
#define TSE 1 << 25 // Transmit Size Error
#define RC 1 << 26 // Receive Complete
#define TC 1 << 27 // Transmit Complete
#define TPOE 1 << 28 // Transmit Packet Overrun Error
#define RPUE 1 << 29 // Receive Packet Underrun Error
#define RPORE 1 << 30 // Receive Packet Overrun Read Error
#define RPURE 1 << 31 // Receive Packet Underrun Read Error

// forward declaration of axi_i2s
struct axi_i2s;

// declare functions we will use from the esli2s driver
extern void esl_codec_enable_tx(struct axi_i2s* i2s);
extern void esl_codec_disable_tx(struct axi_i2s* i2s);

// structure for an individual instance (ie. one audio driver)
struct esl_audio_instance
{
  void* __iomem regs; //fifo registers
  struct cdev chr_dev; //character device
  dev_t devno;

  // list head
  struct list_head inst_list;

  // interrupt number
  unsigned int irqnum;

  // fifo depth
  unsigned int tx_fifo_depth;

  // wait queue
  wait_queue_head_t waitq;

  // i2s controller instance
  struct axi_i2s* i2s_controller;
};

// Utility method to read registers
static inline u32 reg_read(struct esl_audio_instance* inst, u32 reg)
{
  return ioread32(inst->regs + reg);
}

// Utility method to write registers
static inline void reg_write(struct esl_audio_instance* inst, u32 reg,
                             u32 value)
{
  iowrite32(value, inst->regs + reg);
}

static void axi_fifo_init(struct esl_audio_instance* inst)
{
  printk(KERN_INFO "Initializing FIFO\n");

  // Reset FIFO with the TDFR register
  reg_write(inst, TDFR, 0x000000A5); // TDFR Reset

  // Enable and clear FIFO interrupt initially using register ISR and IER
  reg_write(inst, ISR, 0xFFFFFFFF); // ISR Reset interrupt bits
  
  reg_write(inst, IER, 0x00000000); // IER Reset

  // Enable all transmit interrupt bits as well as the receive complete interrupt bit
    // ier_transmit_interrupt_bits = 0x1F600000;
    // ier_receive_interrupt_bits = 0b00011111011000000000000000000000;
    // ier_receieve_interrupt_bits = 526385152
  uint32_t ier_interrupt_enable_bits = TPOE | TC | RC | TSE | TRC | TFPF | TFPE;
  reg_write(inst, IER, ier_interrupt_enable_bits);

  reg_write(inst, TDR, 0x00000002); // TDR Destination address

  printk(KERN_INFO "ISR: %x\n", reg_read(inst, ISR));
  printk(KERN_INFO "IER: %x\n", reg_read(inst, IER));
  printk(KERN_INFO "TDFV: %x\n", reg_read(inst, TDFV));
  printk(KERN_INFO "RDFO: %x\n", reg_read(inst, RDFO));
}


/* Check FIFO vacancy */
unsigned char fifo_full(struct esl_audio_instance* inst)
{
  return reg_read(inst, TDFV);
}

static void fifo_write(struct esl_audio_instance* inst, uint32_t word)
{
  /*
  When presenting a transmit packet to the AXI4-Stream FIFO core, write the destination address 
  into TDR first, write the packet data to the Transmit Data FIFO next, and then write the 
  length of the packet into the TLR.

  The destination address must be written to the TDR before the packet data is written to the
  transmit data FIFO.
  */

  // Write data to FIFO
  reg_write(inst, TDFD, word);
  // Write length of data to TLR
  reg_write(inst, TLR, sizeof(uint32_t));
  // Clear the interrupt status register by writing all 1s
  reg_write(inst, ISR, 0xFFFFFFFF);
  uint32_t isr = reg_read(inst, ISR);
  uint32_t tdfv = reg_read(inst, TDFV);
  //printk(KERN_INFO "isr: %x, tdfv: %x\n", isr, tdfv);
}

// structure for class of all audio drivers
struct esl_audio_driver
{
  dev_t first_devno;
  struct class* class;
  unsigned int instance_count;     // how many drivers have been instantiated?
  struct list_head instance_list;  // pointer to first instance
};

// allocate and initialize global data (class level)
static struct esl_audio_driver driver_data = {
  .instance_count = 0,
  .instance_list = LIST_HEAD_INIT(driver_data.instance_list),
};

/* Utility Functions */

// find instance from inode using minor number and linked list
static struct esl_audio_instance* inode_to_instance(struct inode* i)
{
  struct esl_audio_instance *inst_iter;
  unsigned int minor = iminor(i);

  // start with fist element in linked list (stored at class),
  // and iterate through its elements
  list_for_each_entry(inst_iter, &driver_data.instance_list, inst_list)
    {
      // found matching minor number?
      if (MINOR(inst_iter->devno) == minor)
        {
          // return instance pointer of corresponding instance
          return inst_iter;
        }
    }

  // not found
  return NULL;
}

// return instance struct based on file
static struct esl_audio_instance* file_to_instance(struct file* f)
{
  return inode_to_instance(f->f_path.dentry->d_inode);
}

/* Character device File Ops */
static ssize_t esl_audio_write(struct file* f,
                               const char __user *buf, size_t len,
                               loff_t* offset)
{
  struct esl_audio_instance *inst = file_to_instance(f);
  unsigned int written = 0;

  if (!inst)
    {
      // instance not found
      return -ENOENT;
    }
  // printk(KERN_INFO "esl_audio_write: Writing %zu bytes\n", len);
  static uint32_t kernel_buffer[128];

  size_t bytes_to_copy = min(len, sizeof(kernel_buffer));
  size_t not_copied = copy_from_user(&kernel_buffer, buf, bytes_to_copy);

  if (not_copied) {
      printk(KERN_WARNING "esl_audio_write: Could not copy %zu bytes from user space\n", not_copied);
  }

  size_t bytes_written = 0;
  int words_to_write = bytes_to_copy / sizeof(uint32_t);
  int i;

  for(i = 0; i < words_to_write; i++) {
      while (fifo_full(inst) < 32) {
          //printk(KERN_INFO "esl_audio_write: FIFO is full, waiting\n");
          //usleep_range(0, 100);
          wait_event_interruptible(inst->waitq, fifo_full(inst) >= 32);
      }
      //printk(KERN_INFO "esl_audio_write: writing word: %x\n", kernel_buffer[i]);
      fifo_write(inst, kernel_buffer[i]);
      bytes_written += sizeof(uint32_t);
  }

  return bytes_written;
}

// definition of file operations 
struct file_operations esl_audio_fops = {
  .write = esl_audio_write,
};

/* interrupt handler */
static irqreturn_t esl_audio_irq_handler(int irq, void* dev_id)
{
  struct esl_audio_instance* inst = dev_id;

  // TODO handle interrupts
  uint32_t interrupt_status = reg_read(inst, ISR);
  if (TPOE & interrupt_status) {
    printk(KERN_INFO "esl_audio_irq_handler: Transmit Packet Overrun Error\n");
    reg_write(inst, ISR, TPOE); // TPOE Reset
  }
  if (TC & interrupt_status) {
    printk(KERN_INFO "esl_audio_irq_handler: Transmit Complete Interrupt\n");
    reg_write(inst, ISR, TC); // TC Reset
  }
  if (RC & interrupt_status) {
    printk(KERN_INFO "esl_audio_irq_handler: Receive Complete\n");
    reg_write(inst, ISR, RC); // RC Reset
  }
  if (TSE & interrupt_status) {
    printk(KERN_INFO "esl_audio_irq_handler: Transmit Size Error\n");
    reg_write(inst, ISR, TSE); // TSE Reset
  }
  if (TRC & interrupt_status) {
    printk(KERN_INFO "esl_audio_irq_handler: Transmit Reset Complete\n");
    reg_write(inst, ISR, TRC); // TRC Reset
  }
  if (TFPF & interrupt_status) {
    printk(KERN_INFO "esl_audio_irq_handler: Transmit FIFO Programmable Full\n");
    reg_write(inst, ISR, TFPF); // TFPF Reset
  }
  if (TFPE & interrupt_status) {
    printk(KERN_INFO "esl_audio_irq_handler: Transmit FIFO Programmable Empty\n");
    reg_write(inst, ISR, TFPE); // TFPE Reset
    wake_up_interruptible(&inst->waitq);
  }

  // Clear the ISR by writing all 1s
  //reg_write(inst, ISR, 0xFFFFFFFF);

  return IRQ_HANDLED;
}

static int esl_audio_probe(struct platform_device* pdev)
{
  struct esl_audio_instance* inst = NULL;
  int err;
  struct resource* res;
  const void* prop;
  phandle i2s_phandle;
  struct device_node* i2sctl_node;

  // allocate instance
  inst = devm_kzalloc(&pdev->dev, sizeof(struct esl_audio_instance),
                      GFP_KERNEL);

  // set platform driver data
  platform_set_drvdata(pdev, inst);

  // get registers (AXI FIFO)
  res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (IS_ERR(res))
    {
      return PTR_ERR(res);
    }

  inst->regs = devm_ioremap_resource(&pdev->dev, res);
  if (IS_ERR(inst->regs))
    {
      return PTR_ERR(inst->regs);
    }

  // get TX fifo depth
  err = of_property_read_u32(pdev->dev.of_node, "xlnx,tx-fifo-depth",
                             &inst->tx_fifo_depth);
  if (err)
    {
      printk(KERN_ERR "%s: failed to retrieve TX fifo depth\n",
             DRIVER_NAME);
      return err;
    }

  // get interrupt
  res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
  if (IS_ERR(res))
    {
      return PTR_ERR(res);
    }

  err = devm_request_irq(&pdev->dev, res->start,
                         esl_audio_irq_handler,
                         IRQF_TRIGGER_HIGH,
                         "zedaudio", inst);
  if (err < 0)
    {
      return err;
    }

  // save irq number
  inst->irqnum = res->start;

  // get and inspect i2s reference
  prop = of_get_property(pdev->dev.of_node, "esl,i2s-controller", NULL);
  if (IS_ERR(prop))
    {
      return PTR_ERR(prop);
    }

  // cast into phandle
  i2s_phandle = be32_to_cpu(*(phandle*)prop);
  i2sctl_node = of_find_node_by_phandle(i2s_phandle);
  if (!i2sctl_node)
    {
      // couldnt find
      printk(KERN_ERR "%s: could not find AXI I2S controller", DRIVER_NAME);
      return -ENODEV;
    }

  // get controller instance pointer from device node
  inst->i2s_controller = i2sctl_node->data;
  of_node_put(i2sctl_node);

  // create character device
  // get device number
  inst->devno = MKDEV(MAJOR(driver_data.first_devno),
                      driver_data.instance_count);

  // TODO initialize and create character device
  cdev_init(&inst->chr_dev, &esl_audio_fops);

  // Add character device to the system
  err = cdev_add(&inst->chr_dev, inst->devno, 1);
  if (err < 0) {
      dev_err(&pdev->dev, "Failed to add character device\n");
      return err;
  }

  // Create a device file in /dev
  device_create(driver_data.class, NULL, inst->devno, NULL, "zedaudio%d", driver_data.instance_count);

  // increment instance count
  driver_data.instance_count++;

  // put into list
  INIT_LIST_HEAD(&inst->inst_list);
  list_add(&inst->inst_list, &driver_data.instance_list);

  // init wait queue
  init_waitqueue_head(&inst->waitq);

  // TODO reset AXI FIFO
  axi_fifo_init(inst);
  // TODO enable interrupts
  // int ret = request_threaded_irq(inst->irqnum, esl_audio_irq_handler, NULL, IRQF_TRIGGER_RISING, "esl_audio_irq_handler", NULL);
  // if (ret) 
  // {
  //   printk(KERN_ALERT "Failed to request IRQ %d\n", inst->irqnum);
  //   return -EIO;
  // }
  return 0;
}

static int esl_audio_remove(struct platform_device* pdev)
{
  struct esl_audio_instance* inst = platform_get_drvdata(pdev);

  // Remove the deevice file
  device_destroy(driver_data.class, inst->devno);

  // TODO remove all traces of character device
  cdev_del(&inst->chr_dev);

  driver_data.instance_count--;

  // remove from list
  list_del(&inst->inst_list);

  return 0;
}

// matching table
static struct of_device_id esl_audio_of_ids[] = {
  { .compatible = "esl,audio-fifo" },
  { }
};

// platform driver definition
static struct platform_driver esl_audio_driver = {
  .probe = esl_audio_probe,
  .remove = esl_audio_remove,
  .driver = {
    .name = DRIVER_NAME,
    .of_match_table = of_match_ptr(esl_audio_of_ids),
  },
};

static int esl_audio_init(void)
{
  int err;

  // alocate character device region
  err = alloc_chrdev_region(&driver_data.first_devno, 0, 16, "zedaudio");
  if (err < 0)
    {
      return err;
    }

  // create class
  // although not using sysfs, still necessary in order to automatically
  // get device node in /dev
  driver_data.class = class_create(THIS_MODULE, "zedaudio");
  if (IS_ERR(driver_data.class))
    {
      return -ENOENT;
    }

  platform_driver_register(&esl_audio_driver);

  return 0;
}

static void esl_audio_exit(void)
{
  platform_driver_unregister(&esl_audio_driver);

  // free character device region
  unregister_chrdev_region(driver_data.first_devno, 16);

  // remove class
  class_destroy(driver_data.class);
}

module_init(esl_audio_init);
module_exit(esl_audio_exit);

MODULE_DESCRIPTION("ZedBoard Simple Audio driver");
MODULE_LICENSE("GPL");
