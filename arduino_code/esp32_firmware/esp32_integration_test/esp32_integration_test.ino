#include "../wifi_utils.h"
#include "../camera_utils.h"
#include "../sd_card_utils.h"
#include "../i2s_utils.h"
#include "../max_utils.h"
#include "../gpio_pins.h"

#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <EEPROM.h>            // read and write from flash memory
#include <HTTPClient.h>
#include <FastLED.h>

#define SERVER_URL "http://10.203.140.73:8888/uploadAudio"
#define PHOTO_SERVER_URL "http://10.203.140.73:8888/uploadPhoto"

const char img_file_name[] = "/image.jpg";
const char audio_file_name[] = "/test.wav";

// Variables globales
CRGB leds[NUM_LEDS];   // Control de LEDs
FS* sd;                // Declarar el puntero sd como global

// Prototipos de funciones
void init_wifi();
FS* init_sdcard();
bool init_camera();
bool init_i2s_mic();
bool init_i2s_speaker();
bool deinit_i2s_mic();
bool deinit_i2s_speaker();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Iniciando setup...");

  // Inicializar botones
  pinMode(1, INPUT_PULLUP);
  pinMode(42, INPUT_PULLUP);
  pinMode(43, INPUT_PULLUP);

  // Inicializar WiFi
  init_wifi();
  delay(100);
  Serial.println("WiFi inicializado.");

  // Inicializar tarjeta SD
  Serial.println("Inicializando tarjeta SD...");
  sd = init_sdcard();  // Asignar el puntero sd
  if (!sd) {
    Serial.println("Fallo al inicializar la tarjeta SD!");
    return;
  }
  Serial.println("Tarjeta SD inicializada.");
  delay(100);

  // Inicializar cámara
  if (!init_camera()) {
    Serial.println("Error al inicializar la cámara.");
    return;
  }
  Serial.println("Cámara inicializada.");
  delay(100);

  // Inicializar LEDs
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  leds[0] = CRGB::Green;
  FastLED.show();
  delay(100);
  Serial.println("LEDs inicializados.");

  if (!init_i2s_mic()) {
    Serial.println("Fallo al inicializar el micrófono I2S!");
    return;
  }
  delay(100);

  if (!init_i2s_speaker()) {
    Serial.println("Fallo al inicializar el MAX98357 I2S!");
    return;
  }
  delay(100);

  Serial.println("Setup completado. Esperando acciones del usuario...");
}

void loop() {
  // Aquí va el resto del código principal
  if (digitalRead(1) == LOW) {
    if (!play_audio_from_sd(sd, audio_file_name)) {  // Usar el puntero sd
      Serial.println("Error al reproducir el archivo de audio.");
    }
  }
  // } else if (digitalRead(42) == LOW) {
  //   // Lógica para otro botón
  // }
}

void uploadPhoto(const char *photoPath) {
    if (WiFi.status() != WL_CONNECTED) {
      ets_printf("Error: WiFi no conectado.\n");
      return;
    }
    
    // Abrir la foto y obtener su tamaño
    File photoFile = SD_MMC.open(photoPath);
    if (!photoFile) {
      ets_printf("No se pudo abrir la foto.\n");
      return;
    }
    size_t fileSize = photoFile.size();
  
    // Construir el cuerpo del POST en formato multipart/form-data
    String boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW"; // Cadena única para separar partes
    String multipartHeader = "--" + boundary + "\r\n" +
                             "Content-Disposition: form-data; name=\"file\"; filename=\"photo.jpg\"\r\n" +
                             "Content-Type: image/jpeg\r\n\r\n";
    String multipartFooter = "\r\n--" + boundary + "--\r\n";
  
    int headerLen = multipartHeader.length();
    int footerLen = multipartFooter.length();
    int totalLen = headerLen + fileSize + footerLen;
  
    // Asignar un único buffer para el header, el contenido de la foto y el footer
    uint8_t* postBuffer = (uint8_t*)malloc(totalLen);
    if (!postBuffer) {
      ets_printf("Error: No se pudo asignar memoria para el buffer del POST.\n");
      photoFile.close();
      return;
    }
  
    // Copiar el header al buffer
    memcpy(postBuffer, multipartHeader.c_str(), headerLen);
  
    // Leer el contenido del archivo directamente en el buffer, después del header
    size_t bytesRead = photoFile.read(postBuffer + headerLen, fileSize);
    if (bytesRead != fileSize) {
      ets_printf("Error: No se pudo leer toda la foto.\n");
      free(postBuffer);
      photoFile.close();
      return;
    }
    photoFile.close();
  
    // Copiar el footer al buffer
    memcpy(postBuffer + headerLen + fileSize, multipartFooter.c_str(), footerLen);
  
    // Enviar el POST con el contenido multipart
    WiFiClient client;
    HTTPClient http;
    http.begin(client, PHOTO_SERVER_URL);
  
    String contentType = "multipart/form-data; boundary=" + boundary;
    http.addHeader("Content-Type", contentType);
  
    int httpResponseCode = http.POST(postBuffer, totalLen);
    free(postBuffer);
  
    if (httpResponseCode > 0) {
      ets_printf("Foto subida, código: %d\n", httpResponseCode);
      String response = http.getString();
      ets_printf("Respuesta del servidor: %s\n", response.c_str());
    } else {
      ets_printf("Error en la subida: %d\n", httpResponseCode);
    }
    http.end();
}

void uploadFile(const char *filePath) {
    // Abrir el archivo de audio desde SD_MMC usando el path recibido
    File file = SD_MMC.open(filePath, FILE_READ);
    if (!file) {
      ets_printf("El archivo no está disponible!\n");
      return;
    }
  
    ets_printf("===> Subiendo archivo al servidor\n");
    HTTPClient client;
    client.begin(SERVER_URL); // Servidor de audio
    client.addHeader("Content-Type", "audio/wav");
  
    int httpResponseCode = client.sendRequest("POST", &file, file.size());
    ets_printf("httpResponseCode: %d\n", httpResponseCode);
  
    if (httpResponseCode == 200) {
      String response = client.getString();
      ets_printf("==================== Transcripción ====================\n");
      ets_printf("%s\n", response.c_str());
      ets_printf("====================      Fin      ====================\n");
  
      // Si la transcripción contiene "capturar foto", se invoca la función para capturar y enviar la foto
      if (response.indexOf("capturar foto") >= 0) {
        ets_printf("Transcripción indica 'capturar foto'. Iniciando captura y envío de foto...\n");
      }
    } else {
      ets_printf("Error en la solicitud HTTP\n");
    }
  
    file.close();
    client.end();
}