#include "pico/stdlib.h"
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/uart.h>
#include <pico/i2c_slave.h>
#include <pico/time.h>
#include <stdio.h>
#include <string.h>
#include "i2c.h"


static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
  switch (event) {
  case I2C_SLAVE_RECEIVE:
    if (buf_index >= BUF_SIZE) {
      buf_index = 0;
    }
    uint8_t byte = i2c_read_byte_raw(i2c);
    if (!is_written) {
      reg = byte;
      buf_index = reg % BUF_SIZE;
      is_written = true;
    } else {
      i2c_buf[buf_index] = byte;
      buf_index++;
    }
    break;
  case I2C_SLAVE_REQUEST:
    if (reg < BUF_SIZE) {
      i2c_write_byte_raw(i2c, i2c_buf[reg]);
      if (reg != BUF_SIZE - 1) {
        reg++;
      }
    }
    break;
  case I2C_SLAVE_FINISH:
    is_written = false;
    buf_index = 0;
    break;
  default:
    break;
  }
}

void i2c_slave_setup() {
  i2c_init(SLAVE_I2C_ID, I2C_BAUDRATE);
  gpio_init(SLAVE_I2C_SDA);
  gpio_init(SLAVE_I2C_SCL);
  gpio_set_function(SLAVE_I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(SLAVE_I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(SLAVE_I2C_SDA);
  gpio_pull_up(SLAVE_I2C_SCL);
  i2c_slave_init(SLAVE_I2C_ID, I2C_SLAVE_ADDR, &i2c_slave_handler);
}

void i2c_master_setup() {
  i2c_init(MASTER_I2C_ID, I2C_BAUDRATE);
  gpio_init(MASTER_I2C_SDA);
  gpio_init(MASTER_I2C_SCL);
  gpio_set_function(MASTER_I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(MASTER_I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(MASTER_I2C_SDA);
  gpio_pull_up(MASTER_I2C_SCL);
}

static void i2c_master_send(uint8_t *data, uint8_t len, uint8_t index) {
  int count = 0;
  uint8_t buf[len + 1];
  buf[0] = index;
  memcpy(buf + 1, data, len);
  printf("Sending the following message to Slave: \"%s\"\n", data);

  count =
      i2c_write_blocking(MASTER_I2C_ID, I2C_SLAVE_ADDR, buf, len + 1, false);
  if (count < 0) {
    puts("Error: Couldn't write to the Slave.\n");
  }
  hard_assert(count == len + 1);
}

static void i2c_master_read(uint8_t *data, uint8_t len, uint8_t index) {
  int count = 0;
  puts("Reading message from the Slave\n");

  count = i2c_write_blocking(MASTER_I2C_ID, I2C_SLAVE_ADDR, &index, 1, true);
  if (count < 0) {
    puts("Error: Couldn't write to the Slave.\n");
  }
  hard_assert(count == 1);

  count = i2c_read_blocking(MASTER_I2C_ID, I2C_SLAVE_ADDR, data, len, false);
  if (count < 0) {
    puts("Error: Couldn't read from the Slave.\n");
  }
  hard_assert(count == len);

  printf("Received the following message from the Slave: \"%s\"\n", data);
}

void master_run(uint8_t *data) {
  uint8_t rcv_data[BUF_SIZE] = {0};
  uint8_t index = 0;

  i2c_master_send(data, BUF_SIZE, index);
  i2c_master_read(rcv_data, BUF_SIZE, index);
}
