/* morse_io.c */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
#define class class_simple
#define class_create class_simple_create
#define device_create class_simple_device_add
#define _device_destroy(class, first)\
			class_simple_device_remove(first)
#define class_destroy class_simple_destroy
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,17)
#define device_create class_device_create
#define device_destroy(class, first)\
			class_device_destroy(class, first)
#endif

struct class *my_class;

#define MYDEV_NAME	"morse"
#define MYDEV_DEV	"morse%d"

static dev_t first;
static unsigned int count = 2;
static struct cdev *my_cdev;

#define CLK_FREQ	(1193180L)
#define PIO			(0x61)
#define PIT_CMD		(0x43)
#define PIT_DATA	(0x42)
#define SETUP		(0xB6)
#define TONE_ON		(0x03)
#define TONE_OFF	(0xFC)

void sound(int freq) {
	unsigned int value = inb(PIO);
	freq = CLK_FREQ / freq;
	if ((value & TONE_ON) == 0) {
		outb(value | TONE_ON, PIO);
		outb(SETUP, PIT_CMD);
	}
	outb(freq & 0xff, PIT_DATA);
	outb((freq >> 8) & 0xff, PIT_DATA);
}

void nosound(void) {
	unsigned int value = inb(PIO);
	value &= TONE_OFF;
	outb(value, PIO);
}

#define MORSE_SPACE_MASK	(1 << 15)
#define MORSE_BIT_MASK		(0xFE)
#define MORSE_UNIT_TIME		(60)
#define MORSE_FREQUENCY		(2000)

void send_dot(void) {
	sound(MORSE_FREQUENCY);
	mdelay(MORSE_UNIT_TIME);
	nosound();
	mdelay(MORSE_UNIT_TIME);
}

void send_dash(void) {
	sound(MORSE_FREQUENCY);
	mdelay(MORSE_UNIT_TIME * 3);
	nosound();
	mdelay(MORSE_UNIT_TIME);
}

void letter_space(void) {
	mdelay(MORSE_UNIT_TIME * 2);
}

void word_space(void) {
	mdelay(MORSE_UNIT_TIME * 4);
}

void morse(char *cp) {
	unsigned int c;
	static unsigned int codes[64] = {
		MORSE_SPACE_MASK, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 115, 49, 106, 41,
		63, 62, 60, 56, 48, 32, 33, 35,
		39, 47, 0, 0, 0, 0, 0, 76,
		0, 6, 17, 21, 9, 2, 20, 11,
		16, 4, 30, 13, 18, 7, 5, 15,
		22, 27, 10, 8, 3, 12, 24, 14,
		25, 29, 19
	};
	while ((c = *cp++) != '\0') {
		if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
		c -= ' ';
		if (c > 58) continue;
		c = codes[c];
		if (c & MORSE_SPACE_MASK) {
			word_space(); continue;
		}
		while (c & MORSE_BIT_MASK) {
			if (c & 1) send_dash();
			else send_dot();
			c >>= 1;
		}
		letter_space();
	}
}

ssize_t m_write(struct file *filp, const char *buffer,
		size_t length, loff_t *offset) {
	char *data = (char *)kmalloc(length, GFP_KERNEL);
	if (data == NULL) return 0;
	if (copy_from_user(data, buffer, length) > 0)
		printk("Warning: Failed to copy whole messages, but continuing the operation\n");
	morse(data);
	kfree(data);
	return length;
}

struct file_operations m_fops = {
	.write = m_write
};

int __init init_morse_module(void) {
	int i;
	dev_t node_no;

	if (alloc_chrdev_region(&first, 0, count, MYDEV_NAME) < 0) {
		printk("Failed to allocate character device\n");
		return -1;
	}
	if (!(my_cdev = cdev_alloc())) {
		printk("cdev_alloc() failed\n");
		unregister_chrdev_region(first, count);
		return -1;
	}
	cdev_init(my_cdev, &m_fops);
	if (cdev_add(my_cdev, first, count) < 0) {
		printk("cdev_add() failed\n");
		unregister_chrdev_region(first, count);
		return -1;
	}

	my_class = class_create(THIS_MODULE, MYDEV_NAME);
	for (i = 0; i < count; i++) {
		node_no = MKDEV(MAJOR(first), i);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
		device_create(my_class, node_no, NULL, MYDEV_DEV, i);
#else
		device_create(my_class, NULL, node_no, NULL, MYDEV_DEV, i);
#endif
	}
	return 0;
}

void __exit exit_morse_module(void) {
	int i;
	dev_t node_no;
	for (i = 0; i < count; i++) {
		node_no = MKDEV(MAJOR(first), i);
		device_destroy(my_class, node_no);
	}
	class_destroy(my_class);

	if (my_cdev) {
		cdev_del(my_cdev);
	}
	unregister_chrdev_region(first, count);
}

module_init(init_morse_module);
module_exit(exit_morse_module);

MODULE_LICENSE("GPL");
