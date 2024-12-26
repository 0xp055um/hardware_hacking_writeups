#include "pico/stdlib.h"
#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <pico/time.h>
#include <stdio.h>
#include "i2c.h"

// LED
#define DELAY_MS 1000
static const uint LED_PIN = 25;

// UART
#define UART_ID uart0
#define BAUDRATE 115200

static const uint UART_TX = 0;
static const uint UART_RX = 1;

static uint8_t uart_buf[BUF_SIZE + 1] = {0};
static uint8_t uart_buf_i = 0;
static bool is_uart_buf_rdy = false;

static void on_uart_rx() {
  while ((uart_is_readable(UART_ID))) {
    if (uart_buf_i >= BUF_SIZE ) {
      uart_buf_i = 0;
      is_uart_buf_rdy = true;
      continue;
    }

    uint8_t ch = uart_getc(UART_ID);
    if(is_uart_buf_rdy) {
      continue;
    }
    uart_putc(UART_ID, ch);

    if ((ch == '\r') || (ch == '\n')) {
      uart_buf[uart_buf_i] = '\0';
      uart_buf_i = 0;
      is_uart_buf_rdy = true;
    } else {
      uart_buf[uart_buf_i] = ch;
      uart_buf_i++;
    }
  }
}

int main() {
  sleep_ms(1 * DELAY_MS);

  stdio_init_all();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  uart_init(UART_ID, BAUDRATE);
  gpio_set_function(UART_TX, UART_FUNCSEL_NUM(UART_ID, UART_TX));
  gpio_set_function(UART_RX, UART_FUNCSEL_NUM(UART_ID, UART_RX));

  uart_set_fifo_enabled(UART_ID, false);
  irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);
  irq_set_enabled(UART0_IRQ, true);
  uart_set_irq_enables(UART_ID, true, false);

  i2c_slave_setup();
  i2c_master_setup();

  uart_puts(UART_ID, "\033[1;1H\033[2J");
  puts("Welcome to I2C\r\n");

  while (true) {
    puts("\nInsert a message to send to I2C\n");
    is_uart_buf_rdy = false;
    while (!is_uart_buf_rdy) {
      sleep_ms(100);
    }

    gpio_put(LED_PIN, 1);
    sleep_ms(DELAY_MS);
    puts("Sending message to Slave\n");

    master_run(uart_buf);
    sleep_ms(5 * DELAY_MS);

    gpio_put(LED_PIN, 0);
    sleep_ms(DELAY_MS);
  }
  return 0;
}
