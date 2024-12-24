#ifndef PCA555_h
#define PCA555_H

#include "driver/i2c_master.h"
#include <stdint.h>

enum IO{
    INPUT   = 0b1,
    OUTPUT  = 0b0
};

typedef union {
    struct {
        uint8_t P7:1;
        uint8_t P6:1;
        uint8_t P5:1;
        uint8_t P4:1;
        uint8_t P3:1;
        uint8_t P2:1;
        uint8_t P1:1;
        uint8_t P0:1;
    } reg __attribute__((packed));
    uint8_t unsigned_val;           // Access as packed uint8_t
} port_config_t;

typedef const uint8_t PCA555_reg_t;

//                  REGISTERS       READ / WRITE
extern PCA555_reg_t REG_IN_P0;     // READ
extern PCA555_reg_t REG_IN_P1;     // READ

extern PCA555_reg_t REG_OUT_P0;    // READ / WRITE
extern PCA555_reg_t REG_OUT_P1;    // READ / WRITE

extern PCA555_reg_t REG_CFG_P0;    // READ / WRITE
extern PCA555_reg_t REG_CFG_P1;    // READ / WRITE

void PCA555_write_settings(i2c_master_dev_handle_t i2c_dev_handle, port_config_t p0_config, port_config_t p1_config);
uint8_t PCA555_read_register(i2c_master_dev_handle_t i2c_dev_handle, PCA555_reg_t reg);
bool PCA555_simple_read_input_pin(i2c_master_dev_handle_t i2c_dev_handle, uint8_t port, uint8_t channel);

#endif