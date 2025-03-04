#define BOARD_ESP32S3_WROOM

#ifdef BOARD_ESP32S3_WROOM

// #define I2S_WS         45
// #define I2S_SD         47
// #define I2S_SCK        20
// #define I2S_PORT       I2S_NUM_0

#define SAMPLE_BUFFER_SIZE 512
#define SAMPLE_RATE 24000
// most microphones will probably default to left channel but you may need to tie the L/R pin low
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT
// either wire your microphone to the same pins or change these to match your wiring
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_20
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_45
#define I2S_MIC_SERIAL_DATA GPIO_NUM_47

#endif