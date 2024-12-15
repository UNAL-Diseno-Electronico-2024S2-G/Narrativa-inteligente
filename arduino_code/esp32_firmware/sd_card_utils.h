#include "../sd_card_pins.h"
#include "SD_MMC.h"            // SD Card ESP32


void init_sdcard(){
    SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
    Serial.println("Starting SD Card");
    if(!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)){
        Serial.println("SD Card Mount Failed");
        return;
    }

    uint8_t cardType = SD_MMC.cardType();
    if(cardType == CARD_NONE){
        Serial.println("No SD Card attached");
        return;
    }

    Serial.print("SD_MMC Card Type: ");
    if(cardType == CARD_NONE){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }
}