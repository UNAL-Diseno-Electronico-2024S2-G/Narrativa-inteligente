#define BOARD_ESP32S3_WROOM

#ifdef BOARD_ESP32S3_WROOM

// Tamaño del buffer de audio
#define AUDIO_BUFFER_SIZE 512
#define I2S_NUM         I2S_NUM_0
#define SAMPLE_RATE 8000      // Tasa de muestreo
#define SAMPLE_BUFFER_SIZE 512
#define I2S_BCK         2          // Bit Clock (BCLK)
#define I2S_WS          14         // Word Select (LRC)
#define I2S_DO          19         // Data Out (DIN)

// Definiciones compatibles con el código de reproducción de audio
#define I2S_SPEAKER_SERIAL_CLOCK    I2S_BCK  // Bit Clock (BCLK)
#define I2S_SPEAKER_LEFT_RIGHT_CLOCK I2S_WS  // Word Select (LRC)
#define I2S_SPEAKER_SERIAL_DATA     I2S_DO    // Data Out (DIN)

#endif