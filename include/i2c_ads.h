//***************************************************
// ADS11x5 I2C related parameter file
//***************************************************

// ADS1115 I2C Port
#define ADS_ADDR            0x48   // SENSOR_ADDRESS	0x48 -> ADDR line => Gnd

#define ACK_VAL             0x0    // !< I2C ack value
#define AIN0_CHANNEL        0x4000
#define WRITE_BIT I2C_MASTER_WRITE // !< I2C master write
#define READ_BIT  I2C_MASTER_READ  // !< I2C master read

#define ACK_CHECK_EN	    0x1
#define ACK_CHECK_DIS       0x0
#define NACK_VAL			0x1


// end of i2c_ads.h