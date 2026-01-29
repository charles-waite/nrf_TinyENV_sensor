#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
  while (1) {
    printk("nrf-TinyENV: hello from Zephyr\n");
    k_sleep(K_SECONDS(2));
  }
  return 0;
}
