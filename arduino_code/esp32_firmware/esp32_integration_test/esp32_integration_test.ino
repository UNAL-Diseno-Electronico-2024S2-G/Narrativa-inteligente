#include <wav_header.h>

#include "SD_MMC.h"            // SD Card ESP32
#include "FS.h"
#include "FFat.h"
#include <HTTPClient.h>
#include <string.h>
#include <stdio.h>
#include <FastLED.h>
#include "ESP_I2S.h"

// Incluir librerías para la cámara y la tarjeta SD
#include "../camera_utils.h"    // Función init_camera() y demás
#include "../sd_card_utils.h"   // Función init_sdcard() y demás
#include "../wifi_utils.h"
#include "../i2s_utils.h"
#include "../gpio_pins.h"
#include <EEPROM.h>             // Para llevar el número de foto

#define SERVER_URL "http://10.203.140.73:8888/uploadAudio"
#define PHOTO_SERVER_URL "http://10.203.140.73:8888/uploadPhoto"

#define WAV_HEADER_SIZE 44  // Tamaño típico de cabecera WAV

CRGB leds[NUM_LEDS];

const char img_file_name[] = "/image.jpg";
const char audio_file_name[] = "/recording.wav";

bool recordingInProgress = false;

void setup() {
    Serial.begin(115200);
    ets_printf("Serial inicializado a 115200\n");
    
    // Inicializar FastLED y prender LED en verde (estado inactivo)
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
    leds[0] = CRGB::Green;
    FastLED.show();
    
    // Configurar el pulsador con resistencia interna pull-up
    pinMode(buttonPin, INPUT_PULLUP);
  
    // Inicializar WiFi (suponiendo que la función init_wifi() esté definida en otro lado)
    init_wifi();
    delay(500);
  
    // Inicializar el bus I2S mediante la función modular
    I2SClass* i2s = init_i2s();
    if (!i2s) {
      Serial.println("Error al inicializar el bus I2S!");
      return;
    }
  
    // Inicializar la tarjeta SD usando la función modular
    Serial.println("Inicializando tarjeta SD...");
    FS* sd = init_sdcard();
    if (!sd) {
      Serial.println("Fallo al inicializar la tarjeta SD!");
      return;
    }
    Serial.println("Tarjeta SD inicializada.");
  
    // Uso de las funciones para manipular el sistema de archivos
    listDir(*sd, "/", 0);
    createDir(*sd, "/mydir");
    listDir(*sd, "/", 0);
    removeDir(*sd, "/mydir");
    listDir(*sd, "/", 2);
    writeFile(*sd, "/hello.txt", "Hello ");
    appendFile(*sd, "/hello.txt", "World!\n");
    readFile(*sd, "/hello.txt");
    deleteFile(*sd, "/foo.txt");
    renameFile(*sd, "/hello.txt", "/foo.txt");
    readFile(*sd, "/foo.txt");
    testFileIO(*sd, "/test.txt");
    Serial.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));
  
    // Inicializar la cámara usando init_camera()
    // if (!init_camera()) {
    //     Serial.println("Error al inicializar la cámara.");
    // } else {
    //     Serial.println("Cámara inicializada.");
    //     // Capturar foto
    //     camera_fb_t *fb = esp_camera_fb_get();
    //     if (!fb) {
    //         Serial.println("Error al capturar la foto.");
    //     } else {
    //         Serial.println("Foto capturada, guardando en la tarjeta SD...");
    //         // Guardar la imagen capturada en la SD
    //         writeBinaryFile(*sd, img_file_name, fb->buf, fb->len);
    //         Serial.println("Foto guardada en SD.");
    //         // Liberar el frame buffer de la cámara
    //         esp_camera_fb_return(fb);
    //     }
    // }
    
    // Grabar 5 segundos de audio utilizando la función recordWAV de I2S
    uint8_t* wav_buffer;
    size_t wav_size;
    Serial.println("Grabando 5 segundos de audio...");
    wav_buffer = i2s->recordWAV(5, &wav_size);
  
    // Almacenar el audio grabado en la tarjeta SD
    writeBinaryFile(*sd, "/test.wav", wav_buffer, wav_size);
  
    Serial.println("Aplicación completa.");

    uploadFile("/test.wav");
}
  
void loop() {
    delay(10);
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
  