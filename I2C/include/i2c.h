#ifndef I2C_H
#define I2C_H
#include "pico/stdlib.h"

#define SLAVE_I2C_ID i2c0
#define MASTER_I2C_ID i2c1
#define I2C_BAUDRATE 100 * 1000

static const uint SLAVE_I2C_SDA = 4;
static const uint SLAVE_I2C_SCL = 5;
static const uint MASTER_I2C_SDA = 6;
static const uint MASTER_I2C_SCL = 7;

static const uint8_t I2C_SLAVE_ADDR = 0x17;

// Buffers
#define BUF_SIZE 0x20
static uint8_t i2c_buf[BUF_SIZE + 1] = {0};
static uint8_t buf_index = 0;
static uint8_t reg = 0;
static bool is_written = false;

void i2c_master_setup();
void i2c_slave_setup();
void master_run(uint8_t *data);

#endif
