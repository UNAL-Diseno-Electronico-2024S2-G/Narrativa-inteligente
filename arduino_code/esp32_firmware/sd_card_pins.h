#define BOARD_ESP32S3_WROOM

#ifdef BOARD_ESP32S3_WROOM
#define SD_MMC_CMD 38
#define SD_MMC_CLK 39
#define SD_MMC_D0 40
#else
  #error "SD Card model not selected"
#endif