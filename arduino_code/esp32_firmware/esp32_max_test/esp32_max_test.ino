#include "../wifi_utils.h"
#include "../sd_card_utils.h"
#include "../max_utils.h"

#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <EEPROM.h>   
#include <HTTPClient.h>         // read and write from flash memory

// define the number of bytes you want to access
#define EEPROM_SIZE 1

#define SERVER_URL "http://192.168.1.11:8888/uploadAudio"

// Archivo de audio a reproducir
const char* audioFilePath = "/test.wav";  // Cambia al nombre de tu archivo de audio

void setup() {
  Serial.begin(115200);
  init_wifi();

  // Inicializar la tarjeta SD usando la función modular
  Serial.println("Inicializando tarjeta SD...");
  FS* sd = init_sdcard();
  if (!sd) {
    Serial.println("Fallo al inicializar la tarjeta SD!");
    return;
  }
  Serial.println("Tarjeta SD inicializada.");

  // Inicializar el MAX98357 I2S con la misma configuración que la grabación
  if (!init_i2s_speaker()) {
    Serial.println("Fallo al inicializar el MAX98357 I2S!");
    return;
  }
  Serial.println("MAX98357 I2S inicializado.");

  // Reproducir el archivo de audio usando la función modular
  if (!play_audio_from_sd(sd, audioFilePath)) {
    Serial.println("Error al reproducir el archivo de audio.");
  }
}

void loop() {
  // No es necesario hacer nada en el loop para este ejemplo
}