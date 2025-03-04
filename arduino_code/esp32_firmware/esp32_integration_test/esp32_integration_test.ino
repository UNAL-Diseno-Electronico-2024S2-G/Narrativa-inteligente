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

#define SERVER_URL "http://192.168.1.11:8888/uploadAudio"
#define SERVER_URL_D "http://192.168.1.11:8888/downloadAudio"
#define PHOTO_SERVER_URL "http://192.168.1.11:8888/uploadPhoto"

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
bool capture_and_save_photo(FS* sd, const char* file_name);
void uploadPhoto(const char *photoPath);
void waitAndDownloadAudio(const char* audioFilePath);
void uploadFile(const char *filePath);

void setup() {
  Serial.begin(115200);
  delay(1000);
  ets_printf("Iniciando setup...\n");

  // Inicializar botones
  pinMode(1, INPUT_PULLUP); // Reproducir audio
  pinMode(42, INPUT_PULLUP); // Tomar foto
  pinMode(43, INPUT_PULLUP);

  // Inicializar WiFi
  init_wifi();
  delay(100);
  ets_printf("WiFi inicializado.\n");

  // Inicializar tarjeta SD
  ets_printf("Inicializando tarjeta SD...\n");
  sd = init_sdcard();  // Asignar el puntero sd
  if (!sd) {
    ets_printf("Fallo al inicializar la tarjeta SD!\n");
    return;
  }
  ets_printf("Tarjeta SD inicializada.\n");
  delay(100);

  // Inicializar cámara
  if (!init_camera()) {
    ets_printf("Error al inicializar la cámara.\n");
    return;
  }
  ets_printf("Cámara inicializada.\n");
  delay(100);

  // Inicializar LEDs
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  leds[0] = CRGB::Green;
  FastLED.show();
  delay(100);
  ets_printf("LEDs inicializados.\n");

  if (!init_i2s_mic()) {
    ets_printf("Fallo al inicializar el micrófono I2S!\n");
    return;
  }
  delay(100);

  if (!init_i2s_speaker()) {
    ets_printf("Fallo al inicializar el MAX98357 I2S!\n");
    return;
  }
  delay(100);

  ets_printf("Setup completado. Esperando acciones del usuario...\n");
}

void loop() {
  // Aquí va el resto del código principal
  // if (digitalRead(1) == LOW) {
  //   record_push_audio_to_SD(sd, audio_file_name, 1, 12.0);
  //   uploadFile(audio_file_name);
  //   if (!play_audio_from_sd(sd, audio_file_name)) {  // Usar el puntero sd
  //     ets_printf("Error al reproducir el archivo de audio.\n");
  //   }
  // } else 
  if (digitalRead(42) == LOW) {
    // Lógica para tomar foto
    if (capture_and_save_photo(sd, img_file_name)) {
      uploadPhoto(img_file_name);
    } else {
      ets_printf("Error al capturar o guardar la foto.\n");
    }
  }
}

bool capture_and_save_photo(FS* sd, const char* file_name) {
  // Capturar foto
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    ets_printf("Error al capturar la foto.\n");
    return false;
  }

  // Guardar la foto en la tarjeta SD usando writeBinaryFile
  if (!writeBinaryFile(*sd, file_name, fb->buf, fb->len)) {
    ets_printf("Error al guardar la foto en la SD.\n");
    esp_camera_fb_return(fb);
    return false;
  }

  // Liberar el frame buffer de la cámara
  esp_camera_fb_return(fb);
  ets_printf("Foto capturada y guardada en la SD.\n");
  return true;
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

    // Esperar y descargar el audio generado por el servidor
    ets_printf("Esperando audio generado por el servidor...\n");
    waitAndDownloadAudio("/downloaded_audio.wav");
  } else {
    ets_printf("Error en la subida: %d\n", httpResponseCode);
  }
  http.end();
}

void waitAndDownloadAudio(const char* audioFilePath) {
    unsigned long startTime = millis();
  const unsigned long timeout = 120000; // X minutes
  bool audioDownloaded = false;

  // First call /downloadAudio to start generation
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(SERVER_URL_D);
    int httpCode = http.GET();
    http.end();
    ets_printf("Iniciado proceso de generación de audio: %d\n", httpCode);
  }

  // Then poll /checkAudio until file is ready
  while (millis() - startTime < timeout) {
    if (WiFi.status() != WL_CONNECTED) {
      ets_printf("Error: WiFi no conectado.\n");
      return;
    }

    // Try to download from /checkAudio
    HTTPClient http;
    http.begin("http://192.168.1.11:8888/checkAudio");
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      // Save the audio to SD card
      File file = SD_MMC.open(audioFilePath, FILE_WRITE);
      if (file) {
        http.writeToStream(&file);
        file.close();
        ets_printf("Audio descargado y guardado en: %s\n", audioFilePath);
        audioDownloaded = true;
        break;
      }
    }

    http.end();
    delay(2000);
  }

  if (!audioDownloaded) {
    ets_printf("Tiempo de espera agotado. No se pudo descargar el audio.\n");
  } else {
    // Reproducir el audio descargado
    if (!play_audio_from_sd(sd, audioFilePath)) {
      ets_printf("Error al reproducir el archivo de audio.\n");
    }
  }
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
      if (capture_and_save_photo(sd, img_file_name)) {
        uploadPhoto(img_file_name);
      } else {
        ets_printf("Error al capturar o guardar la foto.\n");
      }
    } else {
      // Si no es "capturar foto", esperar y descargar el audio generado por el servidor
      ets_printf("Esperando audio generado por el servidor...\n");
      waitAndDownloadAudio("/downloaded_audio.wav");
    }
  } else {
    ets_printf("Error en la solicitud HTTP\n");
  }

  file.close();
  client.end();
}