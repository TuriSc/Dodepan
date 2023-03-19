#ifndef CONFIG_H_
#define CONFIG_H_

/* Device identifiers */
#define PROGRAM_NAME            "Dodepan"
#define PROGRAM_VERSION         "2.0.0"
#define PROGRAM_DESCRIPTION     "Digital musical instrument. Touch-enabled, with multiple tunings, pitch bending and Midi out."
#define PROGRAM_URL             "https://turiscandurra.com/circuits"
#define USB_STR_MANUFACTURER    "TuriScandurra"
#define USB_STR_PRODUCT         "Dodepan"

/* I/O */
#define SOUND_PIN                   2   // TODO
#define SOUND_PIN_DESCRIPTION       "Audio output"
#define LED_PIN                     18
#define LED_PIN_DESCRIPTION         "Touch feedback LED"
#define LOW_BATT_LED_PIN            17
#define LOW_BATT_LED_DESCRIPTION    "Low battery LED"
#define LOW_BATT_THRESHOLD          3250 // 3.25V
#define PIN_BATT_LVL                29

#define BTN_KEY_DOWN_PIN            19
#define BTN_KEY_DOWN_DESCRIPTION    "Key down button"
#define BTN_KEY_UP_PIN              20
#define BTN_KEY_UP_DESCRIPTION      "Key up button"
#define BTN_SCALE_DOWN_PIN          21
#define BTN_SCALE_DOWN_DESCRIPTION  "Scale down button"
#define BTN_SCALE_UP_PIN            22
#define BTN_SCALE_UP_DESCRIPTION    "Scale up button"

/* Notes */
#define LOWEST_NOTE                 36
#define HIGHEST_NOTE                110
#define NUM_ROOT_KEYS               48  // Four octaves (for root keys)
#define USE_16BIT_SAMPLES           // Use higher quality samples. You'll need
                                    // a larger flash chip (like W25Q128)

/* GY-521 - MPU6050 accelerometer and gyroscope */
// #define USE_GYRO                  // The IMU is optional, but gives you velocity and detune.
#ifdef USE_GYRO
#define MPU6050_I2C_PORT            i2c1
#define MPU6050_SDA_PIN             6 // i2c1
#define MPU6050_SDA_DESCRIPTION     "MPU6050 SDA"
#define MPU6050_SCL_PIN             7 // i2c1
#define MPU6050_SCL_DESCRIPTION     "MPU6050 SCL"

#define MPU6050_ADDRESS             0x68
#define TAP_SENSITIVITY             5.0f // Lower values trigger a higher velocity.
#define DETUNE_FACTOR               2.0f  // Higher values will detune more when tilted.
#define GRAVITY_CONSTANT            9.80665f // Please do not modify the gravity of the earth!
#define RAD_TO_DEG                  57.295779513f // = 1 / (PI / 180).
#endif

/* 74HC4067 Multiplexer/Demultiplexer */
#define USE_DEMUX                   // The demultiplexer is optional. It gives you a visual
                                    // indication of which key and scale are selected.
#ifdef USE_DEMUX
#define DEMUX_EN                    14
#define DEMUX_EN_DESCRIPTION        "Demux EN"
#define DEMUX_SIG                   16
#define DEMUX_SIG_DESCRIPTION       "Demux SIG"
#define DEMUX_S0                    13
#define DEMUX_S0_DESCRIPTION        "Demux S0"
#define DEMUX_S1                    12
#define DEMUX_S1_DESCRIPTION        "Demux S1"
#define DEMUX_S2                    11
#define DEMUX_S2_DESCRIPTION        "Demux S2"
#define DEMUX_S3                    10
#define DEMUX_S3_DESCRIPTION        "Demux S3"
#endif

/* MPR121 */
#define MPR121_I2C_PORT             i2c0
#define MPR121_SDA_PIN              4 // GP4, i2c0
#define MPR121_SDA_DESCRIPTION      "MPR121 SDA"
#define MPR121_SCL_PIN              5 // GP5, i2c0
#define MPR121_SCL_DESCRIPTION      "MPR121 SCL"

#define MPR121_ADDR                 0x5A
#define MPR121_I2C_FREQ             100000

#define MPR121_TOUCH_THRESHOLD      32
#define MPR121_RELEASE_THRESHOLD    6

#endif /* CONFIG_H_ */
