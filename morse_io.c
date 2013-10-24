/* morse_io.c */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#define CLK_FREQ	(1193180L)
#define PIO		(0x61)
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
#define SPACE_MASK	(1 << 15)
#define BIT_MASK	(0xFE)
#define UNIT_TIME	(60)
#define FREQUENCY	(2000)

void send_dot(void) {
	sound(FREQUENCY);
	mdelay(UNIT_TIME);
	nosound();
	mdelay(UNIT_TIME);
}
void send_dash(void) {
	sound(FREQUENCY);
	mdelay(UNIT_TIME * 3);
	nosound();
	mdelay(UNIT_TIME);
}
void letter_space(void) {
	mdelay(UNIT_TIME * 2);
}
void word_space(void) {
	mdelay(UNIT_TIME * 4);
}
void morse(char *cp) {
	unsigned int c;
	static unsigned int codes[64] = {
		SPACE_MASK, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 115, 49, 106, 41,
		63, 62, 60, 56, 48, 32, 33, 35,
		39, 47, 0, 0, 0, 0, 0, 76,
		0, 6, 17, 21, 9, 2, 20, 11,
		16, 4, 30, 13, 18, 7, 5, 15,
		22, 27, 10, 8, 3, 12, 24, 14,
		25, 29, 19
	};
	while ((c = *cp++) != '�') {
		if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
		c -= ' ';
		if (c > 58) continue;
		c = codes[c];
		if (c & SPACE_MASK) {
			word_space(); continue;
		}
		while (c & BIT_MASK) {
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
	copy_from_user(data, buffer, length);
	morse(data);
	kfree(data);
	return length;
}
struct file_operations m_fops = {
	.write = m_write
};
int major_no = 0;
int init_module() {
	major_no = register_chrdev(0, "morse", &m_fops);
	return 0;
}
void cleanup_module() {
	unregister_chrdev(major_no, "morse");
}
