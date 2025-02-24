#include <driver/i2s.h>
#include "FS.h"
#include "FFat.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <string.h>
#include <stdio.h>
#include <FastLED.h>

// Incluir librerías para la cámara y la tarjeta SD
#include "../camera_utils.h"    // Función init_camera() y demás
#include "../sd_card_utils.h"   // Función init_sdcard() y demás
#include <EEPROM.h>             // Para llevar el número de foto

// -------------------------
// CONFIGURACIONES DEL AUDIO
// -------------------------
const char *ssid = "EMILIO1234";         // Tu SSID
const char *password = "Angelita";   // Tu contraseña
#define SERVER_URL "http://192.168.1.21:8888/uploadAudio"  // Servidor de audio

// -------------------------
// CONFIGURACIÓN PARA ENVÍO DE FOTO
// -------------------------
#define PHOTO_SERVER_URL "http://192.168.1.21:8888/uploadPhoto"
// La foto capturada se guardará con un nombre dinámico ("/pictureX.jpg")

// Configuración de I2S  
#define I2S_WS         47
#define I2S_SD         41   // Reasignado para evitar conflicto con el LED
#define I2S_SCK        42
#define I2S_PORT       I2S_NUM_0
#define I2S_SAMPLE_RATE (16000)
#define I2S_SAMPLE_BITS (16)
#define I2S_READ_LEN   (16 * 1024)
#define RECORD_TIME    (5)   // Tiempo de grabación en segundos
#define I2S_CHANNEL_NUM (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)

// Configuración del pulsador (conectado entre GPIO21 y GND)
const int buttonPin = 21;

// Configuración del LED RGB (FastLED)
#define LED_PIN 48         // LED en PIN48
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];

File file;
const char filename[] = "/recording.wav";
const int headerSize = 44;
bool isWIFIConnected = false;
volatile bool recordingInProgress = false;  // Evita grabaciones simultáneas

// -------------------------
// VARIABLES PARA LA FOTO
// -------------------------
#define EEPROM_SIZE 1
int pictureNumber = 0;

// -------------------------
// DECLARACIÓN DE FUNCIONES
// -------------------------
void FFATInit();
void i2sInit();
void i2s_adc_data_scale(uint8_t *d_buff, uint8_t *s_buff, uint32_t len);
void i2s_adc(void *arg);
void wavHeader(byte *header, int wavSize);
void listFFAT(void);
void wifiConnect(void *pvParameters);
void uploadFile();
void capturePhoto();      // Captura foto y la guarda en la SD
void uploadPhoto(const char *photoPath);  // Envía la foto al servidor

// -------------------------
// SETUP Y LOOP
// -------------------------
void setup() {
  Serial.begin(115200);
  ets_printf("Serial inicializado a 115200\n");

  // Inicializar FastLED y prender LED en verde (estado inactivo)
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  leds[0] = CRGB::Green;
  FastLED.show();

  // Configurar el pulsador con resistencia interna pull-up
  pinMode(buttonPin, INPUT_PULLUP);

  ets_printf("Presiona el pulsador para iniciar la grabación...\n");
}

void loop() {
  // Si no se está grabando y se detecta pulsación (estado LOW)
  if (!recordingInProgress && digitalRead(buttonPin) == LOW) {
    delay(50); // Anti-rebote
    if (digitalRead(buttonPin) == LOW) { // Confirmar pulsación
      recordingInProgress = true;
      ets_printf("Pulsador presionado. Iniciando grabación...\n");

      // Cambiar LED a azul para indicar grabación
      leds[0] = CRGB::Blue;
      FastLED.show();

      // Inicializar sistema de archivos FFAT, I2S y crear tareas para grabar y conectar WiFi
      FFATInit();
      i2sInit();
      xTaskCreate(i2s_adc, "i2s_adc", 4096, NULL, 2, NULL);
      xTaskCreate(wifiConnect, "wifi_Connect", 4096, NULL, 1, NULL);

      // Esperar a que se suelte el botón para evitar múltiples activaciones
      while (digitalRead(buttonPin) == LOW) {
        delay(10);
      }
      
      ets_printf("Grabación iniciada. Espera a que termine...\n");
      delay(1000);
    }
  }
  delay(10);
}

// -------------------------
// FUNCIONES DE AUDIO
// -------------------------

void FFATInit() {
  if (!FFat.begin(true)) {
    ets_printf("¡Error al inicializar FFAT!\n");
    while (1) yield();
  }

  // Borrar archivo previo y crear uno nuevo para grabar
  FFat.remove(filename);
  file = FFat.open(filename, FILE_WRITE);
  if (!file) {
    ets_printf("¡El archivo no está disponible!\n");
  }

  // Escribir cabecera WAV
  byte header[headerSize];
  wavHeader(header, FLASH_RECORD_SIZE);
  file.write(header, headerSize);
  listFFAT();
}

void i2sInit() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}

void i2s_adc_data_scale(uint8_t *d_buff, uint8_t *s_buff, uint32_t len) {
  uint32_t j = 0;
  uint32_t dac_value = 0;
  for (int i = 0; i < len; i += 2) {
    dac_value = ((((uint16_t)(s_buff[i + 1] & 0x0F) << 8) | (s_buff[i])));
    d_buff[j++] = 0;
    d_buff[j++] = dac_value * 256 / 2048;
  }
}

void i2s_adc(void *arg) {
  int i2s_read_len = I2S_READ_LEN;
  int flash_wr_size = 0;
  size_t bytes_read;

  char *i2s_read_buff = (char *)calloc(i2s_read_len, sizeof(char));
  uint8_t *flash_write_buff = (uint8_t *)calloc(i2s_read_len, sizeof(char));

  // Sincronizar el I2S con dos lecturas iniciales
  i2s_read(I2S_PORT, (void *)i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
  i2s_read(I2S_PORT, (void *)i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);

  ets_printf(" *** Grabación iniciada *** \n");
  while (flash_wr_size + i2s_read_len <= FLASH_RECORD_SIZE) {
    i2s_read(I2S_PORT, (void *)i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    i2s_adc_data_scale(flash_write_buff, (uint8_t *)i2s_read_buff, i2s_read_len);
    file.write((const byte *)flash_write_buff, i2s_read_len);
    flash_wr_size += i2s_read_len;
    ets_printf("Grabación de audio: %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
    ets_printf("Stack nunca usado: %u\n", uxTaskGetStackHighWaterMark(NULL));
  }
  ets_printf("¡Grabación finalizada!\n");

  file.close();
  free(i2s_read_buff);
  free(flash_write_buff);
  listFFAT();

  // Una vez grabado, si ya se conectó al WiFi se procede a subir el archivo de audio.
  if (isWIFIConnected) {
    uploadFile();
  }
  
  // Cambiar LED a verde al finalizar
  leds[0] = CRGB::Green;
  FastLED.show();

  // Permitir una nueva grabación
  recordingInProgress = false;
  vTaskDelete(NULL);
}

void wavHeader(byte *header, int wavSize) {
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  unsigned int fileSize = wavSize + headerSize - 8;
  header[4] = (byte)(fileSize & 0xFF);
  header[5] = (byte)((fileSize >> 8) & 0xFF);
  header[6] = (byte)((fileSize >> 16) & 0xFF);
  header[7] = (byte)((fileSize >> 24) & 0xFF);
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 0x10;
  header[17] = 0x00;
  header[18] = 0x00;
  header[19] = 0x00;
  header[20] = 0x01;
  header[21] = 0x00;
  header[22] = 0x01;
  header[23] = 0x00;
  header[24] = 0x80;
  header[25] = 0x3E;
  header[26] = 0x00;
  header[27] = 0x00;
  header[28] = 0x00;
  header[29] = 0x7D;
  header[30] = 0x01;
  header[31] = 0x00;
  header[32] = 0x02;
  header[33] = 0x00;
  header[34] = 0x10;
  header[35] = 0x00;
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  header[40] = (byte)(wavSize & 0xFF);
  header[41] = (byte)((wavSize >> 8) & 0xFF);
  header[42] = (byte)((wavSize >> 16) & 0xFF);
  header[43] = (byte)((wavSize >> 24) & 0xFF);
}

void listFFAT(void) {
  ets_printf("\r\nListado de archivos FFAT:\n");
  static const char line[] = "=================================================";
  ets_printf("%s\n", line);
  ets_printf("  Nombre del archivo                    Tamaño\n");
  ets_printf("%s\n", line);

  fs::File root = FFat.open("/");
  if (!root) {
    ets_printf("Error al abrir el directorio\n");
    return;
  }
  if (!root.isDirectory()) {
    ets_printf("No es un directorio\n");
    return;
  }

  fs::File fileItem = root.openNextFile();
  while (fileItem) {
    if (fileItem.isDirectory()) {
      ets_printf("DIR : %s\n", fileItem.name());
    } else {
      const char *fileName = fileItem.name();
      ets_printf("  %s", fileName);
      int spaces = 33 - strlen(fileName);
      if (spaces < 1) spaces = 1;
      for (int i = 0; i < spaces; i++) {
        ets_printf(" ");
      }
      unsigned int fileSize = fileItem.size();
      char fileSizeStr[16];
      snprintf(fileSizeStr, sizeof(fileSizeStr), "%u", fileSize);
      int sizeLen = strlen(fileSizeStr);
      int sizeSpaces = 10 - sizeLen;
      if (sizeSpaces < 1) sizeSpaces = 1;
      for (int i = 0; i < sizeSpaces; i++) {
        ets_printf(" ");
      }
      ets_printf("%s bytes\n", fileSizeStr);
    }
    fileItem = root.openNextFile();
  }
  ets_printf("%s\n", line);
  ets_printf("\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void wifiConnect(void *pvParameters) {
  isWIFIConnected = false;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    ets_printf(".");
  }
  isWIFIConnected = true;
  ets_printf("WiFi conectado\n");
  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void uploadFile() {
  file = FFat.open(filename, FILE_READ);
  if (!file) {
    ets_printf("¡El archivo no está disponible!\n");
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
      capturePhoto();
    }
  } else {
    ets_printf("Error en la solicitud HTTP\n");
  }
  file.close();
  client.end();
}

// -------------------------
// FUNCIÓN PARA CAPTURAR FOTO Y ENVIARLA
// -------------------------
void capturePhoto() {
  ets_printf("CAPTURA INICIADA\n");
  // Inicializar cámara y SD (asegúrate de que init_camera() e init_sdcard() estén implementadas)
  init_camera();
  init_sdcard();

  // Capturar la imagen con la cámara
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    ets_printf("¡Error al capturar foto!\n");
    return;
  }

  // Usar EEPROM para llevar la cuenta de las fotos guardadas
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0);
  pictureNumber++;
  
  // Construir la ruta usando el número de foto (ej.: "/picture1.jpg", "/picture2.jpg", etc.)
  String path = "/picture" + String(pictureNumber) + ".jpg";
  fs::FS &fs = SD_MMC;
  ets_printf("Guardando foto en: %s\n", path.c_str());
  
  File filePhoto = fs.open(path.c_str(), FILE_WRITE);
  if (!filePhoto) {
    ets_printf("Error al abrir el archivo para escribir la foto\n");
  } else {
    filePhoto.write(fb->buf, fb->len);
    ets_printf("Foto guardada en: %s\n", path.c_str());
    EEPROM.write(0, pictureNumber);
    EEPROM.commit();
  }
  filePhoto.close();
  esp_camera_fb_return(fb);

  // Enviar la foto capturada al servidor
  uploadPhoto(path.c_str());
}

// -------------------------
// FUNCIÓN PARA ENVIAR FOTO AL SERVIDOR
// -------------------------
void uploadPhoto(const char *photoPath) {
  if (WiFi.status() != WL_CONNECTED) {
    ets_printf("⚠️ Error: WiFi no conectado.\n");
    return;
  }
  
  // Abrir la foto y leerla en memoria
  File photoFile = SD_MMC.open(photoPath);
  if (!photoFile) {
    ets_printf("⚠️ No se pudo abrir la foto.\n");
    return;
  }
  size_t fileSize = photoFile.size();
  uint8_t* fileBuffer = (uint8_t*)malloc(fileSize);
  if (!fileBuffer) {
    ets_printf("⚠️ Error: No se pudo asignar memoria para la foto.\n");
    photoFile.close();
    return;
  }
  photoFile.read(fileBuffer, fileSize);
  photoFile.close();

  // Construir el cuerpo del POST en formato multipart/form-data
  String boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW"; // Puede ser cualquier cadena única
  String multipartHeader = "--" + boundary + "\r\n" +
                           "Content-Disposition: form-data; name=\"file\"; filename=\"photo.jpg\"\r\n" +
                           "Content-Type: image/jpeg\r\n\r\n";
  String multipartFooter = "\r\n--" + boundary + "--\r\n";

  int headerLen = multipartHeader.length();
  int footerLen = multipartFooter.length();
  int totalLen = headerLen + fileSize + footerLen;

  uint8_t* postBuffer = (uint8_t*)malloc(totalLen);
  if (!postBuffer) {
    ets_printf("⚠️ Error: No se pudo asignar memoria para el buffer del POST.\n");
    free(fileBuffer);
    return;
  }

  // Copiar el header, luego la foto y finalmente el footer
  memcpy(postBuffer, multipartHeader.c_str(), headerLen);
  memcpy(postBuffer + headerLen, fileBuffer, fileSize);
  memcpy(postBuffer + headerLen + fileSize, multipartFooter.c_str(), footerLen);

  free(fileBuffer);

  WiFiClient client;
  HTTPClient http;
  http.begin(client, PHOTO_SERVER_URL);

  String contentType = "multipart/form-data; boundary=" + boundary;
  http.addHeader("Content-Type", contentType);

  int httpResponseCode = http.POST(postBuffer, totalLen);
  free(postBuffer);

  if (httpResponseCode > 0) {
    ets_printf("✅ Foto subida, código: %d\n", httpResponseCode);
    String response = http.getString();
    ets_printf("📩 Respuesta del servidor: %s\n", response.c_str());
  } else {
    ets_printf("⚠️ Error en la subida: %d\n", httpResponseCode);
  }
  http.end();
}