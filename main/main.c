#include <esp_log.h>
#include <stdint.h>
#include <stdio.h>

#include <PCA555.h>

static const char *TAG = "main";

#define IIC_SCL	0
#define IIC_SDA	1

// Defining the settings for the i2c handle
i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = IIC_SCL,
    .sda_io_num = IIC_SDA,
    .glitch_ignore_cnt = 7
};
i2c_device_config_t i2c_pca555_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x20,
    .scl_speed_hz = 100000,
};

void app_main()
{
	esp_log_level_set("*", ESP_LOG_DEBUG);
	ESP_LOGI(TAG, "MCU has booted up successfully!");

	// Setting-up the used PCA9555
	port_config_t p0_cfg, p1_cfg;
	p0_cfg.unsigned_val = 0b11111111;
	p1_cfg.unsigned_val = 0b11111111;

	i2c_master_dev_handle_t i2c_dev_handle;
	i2c_master_bus_handle_t bus_handle;

	ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
	ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &i2c_pca555_cfg, &i2c_dev_handle));

	PCA555_write_settings(i2c_dev_handle, p0_cfg, p1_cfg);
	
	uint8_t p0_state = PCA555_read_register(i2c_dev_handle, REG_IN_P0);
	uint8_t p1_state = PCA555_read_register(i2c_dev_handle, REG_IN_P1);

	for(int port = 0; port < 2; port++) {
		for(int channel = 0; channel < 8; channel++) {
			PCA555_simple_read_input_pin(i2c_dev_handle, port, channel);
		}
	}

	// Changing settings can be done as follows
	p0_cfg.reg.CJ7 = 0;
	PCA555_write_settings(i2c_dev_handle, p0_cfg, p1_cfg);
}
