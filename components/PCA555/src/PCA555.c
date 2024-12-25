#include "PCA555.h"

#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "PCA555_Driver";

// The registers of the PCA9555, for more information please refer to section 6.2 of the datasheet
PCA555_reg_t REG_IN_P0 = 0x00;
PCA555_reg_t REG_IN_P1 = 0x01;

PCA555_reg_t REG_OUT_P0 = 0x02;
PCA555_reg_t REG_OUT_P1 = 0x03;

PCA555_reg_t REG_POL_INV_P0 = 0x04;
PCA555_reg_t REG_POL_INV_P1 = 0x05;

PCA555_reg_t REG_CFG_P0 = 0x06;
PCA555_reg_t REG_CFG_P1 = 0x07;

/**
 * Reads a register of the PCA9555.
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
 * Checks if the setting requested to the PCA9555 has been set successfully.
 * @param i2c_dev_handle The i2c device handle to write the settings of the PCA555 to.
 * @param reg The register of the PCA9555 to read.
 * @param requested_settings The desired settings for the register in the PCA9555.
 * @returns True on successful, false on unsuccessful.
 */
bool PCA555_check_loaded(i2c_master_dev_handle_t i2c_dev_handle, PCA555_reg_t reg, uint8_t requested_settings) {
	// Requesting the settings of the PCA9555 to confirm if the settings have been set correctly
	uint8_t loaded_settings = PCA555_read_register(i2c_dev_handle, reg);
	if( loaded_settings == requested_settings ) {
		ESP_LOGI(TAG, "PCA9555 settings loaded successfully!");
		return true;
	} else {
		ESP_LOGE(TAG, "PCA9555 settings have not been loaded successfully, set to 0x%X instead of 0x%X. Check the I2C connection to make sure it is connected properly!", loaded_settings, requested_settings);
		return false;
	}
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
	PCA555_check_loaded(i2c_dev_handle, REG_CFG_P0, p0_config.unsigned_val);
	PCA555_check_loaded(i2c_dev_handle, REG_CFG_P1, p1_config.unsigned_val);
}

/**
 * Super simplified function to read the port of a pin on the PCA9555.
 * @param i2c_dev_handle The i2c device handle to write the settings of the PCA555 to.
 * @param port	The port to read.
 * @param channel The channel to read.
 * @warning Super inefficient function!
 */
bool PCA555_simple_read_input_pin(i2c_master_dev_handle_t i2c_dev_handle, uint8_t port, uint8_t channel){
	if ( (port > 1) | (channel > 7) ) {
		ESP_LOGE(TAG, "%s Port %i or Channel %i is out of bounds! Can only read P0 to P1 or CH0 to CH7.", __FUNCTION__, port, channel);
	}

	uint8_t port_reading;
	if(port == 0){
		port_reading = PCA555_read_register(i2c_dev_handle, REG_IN_P0);
	}else{
		port_reading = PCA555_read_register(i2c_dev_handle, REG_IN_P1);
	}
	
	bool state = ( (port_reading >> channel ) & 0b00000001 );
	ESP_LOGD(TAG, "%s P%i Channel %i is set to %s.", __FUNCTION__, port, channel, state?"TRUE":"FALSE");
	return state;
}

/**
 * Super simplified function to set the output state of a pin on the PCA9555 without disturbing other registers.
 * @param i2c_dev_handle The i2c device handle to write the settings of the PCA555 to.
 * @param port	The port to read.
 * @param channel The channel to read.
 * @param state The state to set the pin. True for logic high, false for logic low.
 * @warning Super inefficient function! Also sends an IIC message if nothing needs to be changed for given state. 
 * If pin is set to logic HIGH and it already is at logic HIGH, the IIC bus will still send the request to update the value.
 */
bool PCA555_simple_write_output_pin(i2c_master_dev_handle_t i2c_dev_handle, uint8_t port, uint8_t channel, bool state){
	if ( (port > 1) | (channel > 7) ) {
		ESP_LOGE(TAG, "%s Port %i or Channel %i is out of bounds! Can only read P0 to P1 or CH0 to CH7.", __FUNCTION__, port, channel);
	}

	// Requesting the current ouptput settings
	uint8_t output_sett, output_reg;
	if(port == 0){
		output_sett = PCA555_read_register(i2c_dev_handle, REG_OUT_P0);
		output_reg = REG_CFG_P0;
	}else{
		output_sett = PCA555_read_register(i2c_dev_handle, REG_OUT_P1);
		output_reg = REG_CFG_P1;
	}

	// Generating the new values for in the registers
	output_sett = ( output_sett & ~(1 << channel) ) | (state << channel);
	uint8_t i2c_config_output[2] = {output_reg, output_sett};
	ESP_ERROR_CHECK(i2c_master_transmit(i2c_dev_handle, i2c_config_output, 2, -1) );

	return PCA555_check_loaded(i2c_dev_handle, output_sett, output_reg);
}

/**
 * Super simplified function to invert the input port register logic on the PCA9555. 
 * If inverted, incoming logic high becomes incoming logic low.
 * @param i2c_dev_handle The i2c device handle to write the settings of the PCA555 to.
 * @param port	The port to read.
 * @param channel The channel to read.
 * @param state The state to set the pin. True for logic high, false for logic low.
 * @warning Super inefficient function! Also sends an IIC message if nothing needs to be changed for given state. 
 * If pin is set to logic HIGH and it already is at logic HIGH, the IIC bus will still send the request to update the value.
 */
bool PCA555_invert_polarity(i2c_master_dev_handle_t i2c_dev_handle, uint8_t port, uint8_t channel, bool state){
	if ( (port > 1) | (channel > 7) ) {
		ESP_LOGE(TAG, "%s Port %i or Channel %i is out of bounds! Can only read P0 to P1 or CH0 to CH7.", __FUNCTION__, port, channel);
	}

	// Requesting the current ouptput settings
	uint8_t inv_sett, inv_reg;
	if(port == 0){
		inv_sett = PCA555_read_register(i2c_dev_handle, REG_POL_INV_P0);
		inv_reg = REG_POL_INV_P0;
	}else{
		inv_sett = PCA555_read_register(i2c_dev_handle, REG_POL_INV_P1);
		inv_reg = REG_POL_INV_P1;
	}

	// Generating the new values for in the registers
	inv_sett = ( inv_sett & ~(1 << channel) ) | (state << channel);
	uint8_t i2c_config_inv[2] = {inv_reg, inv_sett};
	ESP_ERROR_CHECK(i2c_master_transmit(i2c_dev_handle, i2c_config_inv, 2, -1) );

	return PCA555_check_loaded(i2c_dev_handle, inv_sett, inv_reg);
}
