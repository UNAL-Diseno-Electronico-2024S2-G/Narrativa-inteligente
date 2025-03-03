#include "driver/i2s.h"
#include "../i2s_pins.h"

// Buffer para almacenar las muestras de audio
int16_t i2s_readraw_buff[SAMPLE_BUFFER_SIZE];

bool init_i2s_mic() {
  // Configurar I2S
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_MIC_CHANNEL,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA
  };

  // Instalar y configurar el driver I2S
  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Error al instalar el driver I2S: %d\n", err);
    return false;
  }

  err = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("Error al configurar los pines I2S: %d\n", err);
    return false;
  }

  return true;
}

void writeWavHeader(File file, int sampleRate, int bitsPerSample, int channels, uint32_t dataSize) {
  byte header[44];
  int byteRate = sampleRate * channels * (bitsPerSample / 8);
  int blockAlign = channels * (bitsPerSample / 8);

  // RIFF header
  header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
  uint32_t fileSize = 36 + dataSize; // Tamaño total del archivo
  header[4] = (byte)(fileSize & 0xFF);
  header[5] = (byte)((fileSize >> 8) & 0xFF);
  header[6] = (byte)((fileSize >> 16) & 0xFF);
  header[7] = (byte)((fileSize >> 24) & 0xFF);
  header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';
  // fmt subchunk
  header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
  header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;
  header[20] = 1; header[21] = 0;
  header[22] = channels;
  header[23] = 0;
  header[24] = (byte)(sampleRate & 0xFF);
  header[25] = (byte)((sampleRate >> 8) & 0xFF);
  header[26] = (byte)((sampleRate >> 16) & 0xFF);
  header[27] = (byte)((sampleRate >> 24) & 0xFF);
  header[28] = (byte)(byteRate & 0xFF);
  header[29] = (byte)((byteRate >> 8) & 0xFF);
  header[30] = (byte)((byteRate >> 16) & 0xFF);
  header[31] = (byte)((byteRate >> 24) & 0xFF);
  header[32] = blockAlign;
  header[33] = 0;
  header[34] = bitsPerSample;
  header[35] = 0;
  // data subchunk
  header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
  header[40] = (byte)(dataSize & 0xFF);
  header[41] = (byte)((dataSize >> 8) & 0xFF);
  header[42] = (byte)((dataSize >> 16) & 0xFF);
  header[43] = (byte)((dataSize >> 24) & 0xFF);

  file.write(header, 44);
}

void record_audio_to_SD(FS* sd, const char* filename, uint32_t record_time_ms, float gain = 1.0) {
  size_t bytes_read;
  File file = sd->open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Error al abrir el archivo para escritura!");
    return;
  }

  // Escribir el encabezado WAV (tamaño de datos inicialmente 0)
  writeWavHeader(file, SAMPLE_RATE, 16, 1, 0);

  // Variables para diagnóstico
  int16_t max_sample = -32768;
  int16_t min_sample = 32767;

  // Grabar durante el tiempo especificado
  unsigned long start_time = millis();
  uint32_t total_bytes_written = 0;
  while (millis() - start_time < record_time_ms) {
    i2s_read(I2S_NUM_0, i2s_readraw_buff, SAMPLE_BUFFER_SIZE * sizeof(int16_t), &bytes_read, portMAX_DELAY);

    // Aplicar ganancia a las muestras de audio
    for (size_t i = 0; i < bytes_read / sizeof(int16_t); i++) {
      int32_t sample = i2s_readraw_buff[i] * gain; // Multiplicar por el factor de ganancia

      // Limitar el valor para evitar saturación
      if (sample > 32767) {
        sample = 32767;
      } else if (sample < -32768) {
        sample = -32768;
      }

      i2s_readraw_buff[i] = (int16_t)sample; // Guardar la muestra ajustada

      // Actualizar valores máximo y mínimo para diagnóstico
      if (i2s_readraw_buff[i] > max_sample) {
        max_sample = i2s_readraw_buff[i];
      }
      if (i2s_readraw_buff[i] < min_sample) {
        min_sample = i2s_readraw_buff[i];
      }
    }

    file.write((uint8_t*)i2s_readraw_buff, bytes_read);
    total_bytes_written += bytes_read;
  }

  // Mostrar información de diagnóstico
  Serial.printf("Máxima muestra: %d\n", max_sample);
  Serial.printf("Mínima muestra: %d\n", min_sample);

  // Actualizar el encabezado WAV con el tamaño correcto de los datos
  file.seek(0); // Volver al inicio del archivo
  writeWavHeader(file, SAMPLE_RATE, 16, 1, total_bytes_written);

  file.close();
  Serial.println("Grabación finalizada.");
}