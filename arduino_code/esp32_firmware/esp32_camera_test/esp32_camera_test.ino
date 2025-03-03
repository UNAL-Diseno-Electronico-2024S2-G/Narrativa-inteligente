#include "../wifi_utils.h"
#include "../camera_utils.h"
#include "../sd_card_utils.h"

#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <EEPROM.h>            // read and write from flash memory

// define the number of bytes you want to access
#define EEPROM_SIZE 1

int pictureNumber = 0;

const char img_file_name[] = "/image.jpg";

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

  if (!init_camera()) {
      Serial.println("Error al inicializar la cámara.");
  } else {
      Serial.println("Cámara inicializada.");
      // Capturar foto
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
          Serial.println("Error al capturar la foto.");
      } else {
          Serial.println("Foto capturada, guardando en la tarjeta SD...");
          // Guardar la imagen capturada en la SD
          writeBinaryFile(*sd, img_file_name, fb->buf, fb->len);
          Serial.println("Foto guardada en SD.");
          // Liberar el frame buffer de la cámara
          esp_camera_fb_return(fb);
      }
  }

  delay(2000);
  Serial.println("Going to sleep now");
  delay(2000);
  esp_deep_sleep_start();
  Serial.println("This will never be printed");

}

void loop() {

}