#define BOARD_ESP32S3_WROOM

#ifdef BOARD_ESP32S3_WROOM
// #define I2S_WS         47
// #define I2S_SD         41
// #define I2S_SCK        42
#define I2S_WS         45
#define I2S_SD         47
#define I2S_SCK        20
#define I2S_PORT       I2S_NUM_0
// #define I2S_SAMPLE_RATE (16000)
// #define I2S_SAMPLE_BITS (16)
// #define I2S_READ_LEN   (16 * 1024)
// #define RECORD_TIME    (5)
// #define I2S_CHANNEL_NUM (1)
// #define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)
#endif