#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

int main(void)
{
    printk("Hello Zephyr!\n");

    for(; ; ) {
        // pretending running on MCU
    }
}