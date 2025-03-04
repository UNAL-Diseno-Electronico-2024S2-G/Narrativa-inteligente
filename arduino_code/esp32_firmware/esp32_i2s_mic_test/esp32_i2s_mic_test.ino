#include "../wifi_utils.h"
#include "../sd_card_utils.h"
#include "../i2s_utils.h"

#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <EEPROM.h>   
#include <HTTPClient.h>         // read and write from flash memory

// define the number of bytes you want to access
#define EEPROM_SIZE 1

#define SERVER_URL "http://10.203.186.190:8888/uploadAudio"

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

  // Inicializar el micrófono I2S
  if (!init_i2s_mic()) {
    Serial.println("Fallo al inicializar el micrófono I2S!");
    return;
  }
  Serial.println("Micrófono I2S inicializado.");

  // Grabar audio en la tarjeta SD
  record_audio_to_SD(sd, "/test.wav", 5000, 12.0); // Grabar durante 5 segundos

  // Subir el archivo al servidor
  uploadFile("/test.wav");
}

void loop() {
  // No es necesario hacer nada en el loop para este ejemplo
}

void uploadFile(const char *filePath) {
  // Abrir el archivo de audio desde SD_MMC usando el path recibido
  File file = SD_MMC.open(filePath, FILE_READ);
  if (!file) {
    Serial.println("El archivo no está disponible!");
    return;
  }

  Serial.println("===> Subiendo archivo al servidor");
  HTTPClient client;
  client.begin(SERVER_URL); // Servidor de audio
  client.addHeader("Content-Type", "audio/wav");

  int httpResponseCode = client.sendRequest("POST", &file, file.size());
  Serial.printf("httpResponseCode: %d\n", httpResponseCode);

  if (httpResponseCode == 200) {
    String response = client.getString();
    Serial.println("==================== Transcripción ====================");
    Serial.println(response);
    Serial.println("====================      Fin      ====================");

    // Si la transcripción contiene "capturar foto", se invoca la función para capturar y enviar la foto
    if (response.indexOf("capturar foto") >= 0) {
      Serial.println("Transcripción indica 'capturar foto'. Iniciando captura y envío de foto...");
    }
  } else {
    Serial.println("Error en la solicitud HTTP");
  }

  file.close();
  client.end();
}