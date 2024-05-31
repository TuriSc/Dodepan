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
#define LED_PIN                     18
#define LED_DESCRIPTION             "Touch feedback LED"
#define LOW_BATT_LED_PIN            17
#define LOW_BATT_LED_DESCRIPTION    "Low battery LED"
#define LOW_BATT_THRESHOLD          3250 // 3.25V
#define PIN_BATT_LVL                29

#define ENCODER_DT_PIN              8
#define ENCODER_DT_DESCRIPTION      "Encoder data pin"
#define ENCODER_CLK_PIN             9
#define ENCODER_CLK_DESCRIPTION     "Encoder clock pin"
#define ENCODER_SWITCH_PIN          3
#define ENCODER_SWITCH_DESCRIPTION  "Encoder switch"
#define DOUBLE_CLICK_THRESHOLD_US   250*1000 // 250ms, maximum interval between two button
                                             // presses to trigger a double-click

/* GY-521 - MPU6050 accelerometer and gyroscope */
#define USE_GYRO                    // The IMU is optional, but gives you velocity and bending.
#define MPU6050_I2C_PORT            i2c1
#define MPU6050_SDA_PIN             6 // i2c1
#define MPU6050_SDA_DESCRIPTION     "MPU6050 SDA"
#define MPU6050_SCL_PIN             7 // i2c1
#define MPU6050_SCL_DESCRIPTION     "MPU6050 SCL"

#define MPU6050_ADDRESS             0x68
#define TAP_SENSITIVITY             5.0f // Lower values trigger a higher velocity.
#define BENDING_FACTOR              2.0f  // Higher values will bend more when tilted.
#define REST_ANGLE                  90.0f // Change to 0.0f if your sensor is mounted upright.
#define GRAVITY_CONSTANT            9.80665f // Please do not modify the gravity of the earth!
#define RAD_TO_DEG                  57.295779513f // = 1 / (PI / 180).

/* 74HC4067 Multiplexer/Demultiplexer */
// The demultiplexer gives you a visual
// indication of which key and scale are selected.
#define DEMUX_EN                    16
#define DEMUX_EN_DESCRIPTION        "Demux EN"
#define DEMUX_SIG                   13
#define DEMUX_SIG_DESCRIPTION       "Demux SIG"
#define DEMUX_S0                    14
#define DEMUX_S0_DESCRIPTION        "Demux S0"
#define DEMUX_S1                    15
#define DEMUX_S1_DESCRIPTION        "Demux S1"
#define DEMUX_S2                    11
#define DEMUX_S2_DESCRIPTION        "Demux S2"
#define DEMUX_S3                    12
#define DEMUX_S3_DESCRIPTION        "Demux S3"

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

/* Audio */
#define SOUND_OUTPUT_FREQUENCY      22050

#if USE_AUDIO_PWM
    #define PWM_AUDIO_PIN_RIGHT     1
    #define PWM_AUDIO_RIGHT_DESCRIPTION "Right PWM audio output"
    #define PWM_AUDIO_PIN_LEFT      0
    #define PWM_AUDIO_LEFT_DESCRIPTION  "Left PWM audio output"
#elif USE_AUDIO_I2S
    #include "sound_i2s.h"
    #define I2S_DATA_PIN             2 // -> I2S DIN
    #define I2S_DATA_DESCRIPTION     "I2S DIN"
    #define I2S_CLOCK_PIN_BASE       0 // -> I2S BCK
    #define I2S_BCK_DESCRIPTION      "I2S BCK"
    #define I2S_LRCK_DESCRIPTION     "I2S LRCK" // Must be BCK+1
    // The third required connection is GPIO 1 -> I2S LRCK (BCK+1)
#endif

#endif /* CONFIG_H_ */