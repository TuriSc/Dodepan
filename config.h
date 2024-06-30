#ifndef CONFIG_H_
#define CONFIG_H_

/* Device identifiers */
#define PROGRAM_NAME            "Dodepan"
#define PROGRAM_VERSION         "2.1.1b"
#define PROGRAM_DESCRIPTION     "Digital musical instrument. Touch-enabled, with multiple tunings, pitch bending and Midi out."
#define PROGRAM_URL             "https://turiscandurra.com/circuits"
#define USB_STR_MANUFACTURER    "TuriScandurra"
#define USB_STR_PRODUCT         "Dodepan"

/* I/O */
#define BATT_LVL_PIN                29
#define BATT_LVL_DESCRIPTION        "Battery sensing pin"

#define ENCODER_CLK_PIN             19
#define ENCODER_CLK_DESCRIPTION     "Encoder clock pin"
#define ENCODER_DT_PIN              20
#define ENCODER_DT_DESCRIPTION      "Encoder data pin"
#define ENCODER_SWITCH_PIN          21
#define ENCODER_SWITCH_DESCRIPTION  "Encoder switch"
#define DOUBLE_CLICK_THRESHOLD_US   250*1000 // 250ms, maximum interval between two button
                                             // presses to trigger a double-click

/* SSD1306 Display */
#define USE_DISPLAY
#define SSD1306_I2C_PORT            i2c1
#define SSD1306_SDA_PIN             6 // i2c1
#define SSD1306_SDA_DESCRIPTION     "SSD1306 SDA"
#define SSD1306_SCL_PIN             7 // i2c1
#define SSD1306_SCL_DESCRIPTION     "SSD1306 SCL" // SCL or SCK
#define SSD1306_ADDRESS             0x3C
#define SSD1306_WIDTH               128
#define SSD1306_HEIGHT              32
#define SSD1306_I2C_FREQ            400 * 1000 // 400kHz

/* GY-521 - MPU6050 accelerometer and gyroscope */
// Note: the display and the MPU share the same I²S bus and pins
#define USE_GYRO                    // The IMU is optional, but gives you velocity and bending
#define MPU6050_I2C_PORT            i2c1
#define MPU6050_SDA_PIN             SSD1306_SDA_PIN
#define MPU6050_SDA_DESCRIPTION     "MPU6050 SDA"
#define MPU6050_SCL_PIN             SSD1306_SCL_PIN
#define MPU6050_SCL_DESCRIPTION     "MPU6050 SCL"

#define MPU6050_ADDRESS             0x68
#define MPU6050_SWAP_X_Y            true

#define PLUS " + "
#define SSD1306_MPU6050_SDA_DESCRIPTION SSD1306_SDA_DESCRIPTION PLUS MPU6050_SDA_DESCRIPTION
#define SSD1306_MPU6050_SCL_DESCRIPTION SSD1306_SCL_DESCRIPTION PLUS MPU6050_SCL_DESCRIPTION

#define VELOCITY_MULTIPLIER         8 // Higher values yield higher velocity, but
                                      // lower the dynamic range.

/* MPR121 */
#define MPR121_I2C_PORT             i2c0
#define MPR121_SDA_PIN              4 // i2c0
#define MPR121_SDA_DESCRIPTION      "MPR121 SDA"
#define MPR121_SCL_PIN              5 // i2c0
#define MPR121_SCL_DESCRIPTION      "MPR121 SCL"

#define MPR121_ADDRESS              0x5A
#define MPR121_I2C_FREQ             400 * 1000 // 100kHz

#define MPR121_TOUCH_THRESHOLD      32
#define MPR121_RELEASE_THRESHOLD    6

/* Audio and synth */
#define PRA32_U_MIDI_CH             0  // 0-based

#define AUDIO_BUFFER_LENGTH         64
#define SOUND_OUTPUT_FREQUENCY      48000
#define PICO_AUDIO_I2S_MONO_OUTPUT

#define I2S_PIO_NUM                 0 // 0 for pio0, 1 for pio1
#define I2S_DATA_PIN                2 // -> I2S DIN
#define I2S_DATA_DESCRIPTION        "I2S DIN"
#define I2S_CLOCK_PIN_BASE          0 // -> I2S BCK
#define I2S_BCK_DESCRIPTION         "I2S BCK"
#define I2S_LRCK_DESCRIPTION        "I2S LRCK" // Must be BCK+1
// The third required connection is GPIO 1 -> I2S LRCK (BCK+1)

#define VOL_MIN                     15 // Lowest output volume
#define VOL_INCR                    16 // Volume change amount on a 0-127 scale

#define LPF_MIN                     64 // Lowest filter cutoff frequency on a 0-127 scale.
                                       // Affects the Dodepan preset only.

/* Flash memory */
// Reserve the last 4KB of the default 2MB flash for persistence.
#define FLASH_TARGET_OFFSET         (FLASH_SECTOR_SIZE * 511)
#define MAGIC_NUMBER                {0x44, 0x4F, 0x44, 0x45} // 'DODE' - δώδε means 'twelve' in ancient Greek
#define MAGIC_NUMBER_LENGTH         4
#define FLASH_WRITE_DELAY_S         2  // To minimize flash operations, delay writing by this amount of seconds
#endif /* CONFIG_H_ */
