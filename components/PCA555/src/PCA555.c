#include "PCA555.h"

#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "PCA555_Driver";

// The registers of the PCA9555, for more information please refer to section 6.2 of the datasheet
PCA555_reg_t REG_IN_P0 = 0x00;
PCA555_reg_t REG_IN_P1 = 0x01;

PCA555_reg_t REG_OUT_P0 = 0x02;
PCA555_reg_t REG_OUT_P1 = 0x03;

PCA555_reg_t REG_CFG_P0 = 0x06;
PCA555_reg_t REG_CFG_P1 = 0x07;

/**
 * Rseads a register of the PCA9555.
 * @param i2c_dev_handle The i2c device handle to write the settings of the PCA555 to.
 * @param reg The register of the PCA9555 to read.
 */
uint8_t PCA555_read_register(i2c_master_dev_handle_t i2c_dev_handle, PCA555_reg_t reg) {
	uint8_t i2c_request_reg[1] = {reg};
	uint8_t reg_sett;

	// Requesting the settings of the PCA9555 to confirm if the settings have been set correctly
	ESP_ERROR_CHECK(i2c_master_transmit_receive(i2c_dev_handle, i2c_request_reg, 1, &reg_sett, sizeof(reg), -1) );

	return reg_sett;
}

/**
 * Writes the given settings to the configuration registers of the module.
 * @param i2c_dev_handle The i2c device handle to write the settings of the PCA555 to.
 * @param p0_config	The settings for p0 o ftype port_config.
 * 
 * @note Refer to the PCA9555 datasheet for a detailed overview of what bit is responsible for what register.
 * The default value for each port configuration is set to 1, which is input. 
 * Bit	| 7	| 6	| 5	| 4	| 3	| 2	| 1	| 0	|
 * --------------------------------------
 * P0	|C07|C06|C05|C04|C03|C02|C01|C00|
 * P1	|C17|C16|C15|C14|C13|C12|C11|C10|
 * 
 * @warning If a port is set to an output, the GPIO MUX will automatically set it to the state defined in REG_OUT_P0 and REG_OUT_P1.
 */	
void PCA555_write_settings(i2c_master_dev_handle_t i2c_dev_handle, 
							port_config_t p0_config, port_config_t p1_config) {
	// Defining the messages to be sent over I2C to write the given settings to the PCA9555
	// This can be done way more efficiently, this is merely for keeping the overview for easier debugging with a logic analyzer
	uint8_t i2c_config_p0[2] = {REG_CFG_P0, p0_config.unsigned_val};
	uint8_t i2c_config_p1[2] = {REG_CFG_P1, p1_config.unsigned_val};

	// Transmit the settings over I2C
	ESP_ERROR_CHECK(i2c_master_transmit(i2c_dev_handle, i2c_config_p0, 2, -1) );
	ESP_ERROR_CHECK(i2c_master_transmit(i2c_dev_handle, i2c_config_p1, 2, -1) );

	// Requesting the settings of the PCA9555 to confirm if the settings have been set correctly
	uint8_t p0_config_set = PCA555_read_register(i2c_dev_handle, REG_CFG_P0);
	uint8_t p1_config_set = PCA555_read_register(i2c_dev_handle, REG_CFG_P1);

	if( ( p0_config_set == p0_config.unsigned_val ) && ( p1_config_set == p1_config.unsigned_val )) {
		ESP_LOGI(TAG, "PCA9555 settings loaded successfully!");
		ESP_LOGD(TAG, "P0:\t0x%X\tP1:\t0x%X\t", p0_config_set, p1_config_set);
	} else {
		ESP_LOGE(TAG, "PCA9555 settings have not been loaded successfully.");
		ESP_LOGD(TAG, "P0:\t0x%X\tP1:\t0x%X\t", p0_config_set, p1_config_set);
	}
}

/**
 * Super simplified function to read the port of a pin on the PCA9555.
 * @param i2c_dev_handle The i2c device handle to write the settings of the PCA555 to.
 * @param port	The port to read.
 * @param channel The channel to read.
 * @warning Super inefficient function!
 */
bool PCA555_simple_read_input_pin(i2c_master_dev_handle_t i2c_dev_handle, uint8_t port, uint8_t channel){
	if (port > 1) {
		ESP_LOGE(TAG, "%s Port %i is out of bounds! Can only read P0 or P1.", __FUNCTION__, port);
	}
	if(channel > 7) {
		ESP_LOGE(TAG, "%s Channel %i is out of bounds! Can only read CH0-7.", __FUNCTION__, channel);
	}

	uint8_t port_reading;
	if(port == 0){
		port_reading = PCA555_read_register(i2c_dev_handle, REG_IN_P0);
	}else{
		port_reading = PCA555_read_register(i2c_dev_handle, REG_IN_P1);
	}
	
	bool state = ( (port_reading >> channel ) & 0b00000001 );
	ESP_LOGD(TAG, "State of P%i\tCH%i is set to %s.", port, channel, state?"TRUE":"FALSE");
	return state;
}