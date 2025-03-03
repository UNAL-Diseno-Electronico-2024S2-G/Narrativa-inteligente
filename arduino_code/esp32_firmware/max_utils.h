#include "driver/i2s.h"
#include "../max_pins.h"

// Buffer para almacenar las muestras de audio
int16_t i2s_writeraw_buff[SAMPLE_BUFFER_SIZE];

bool init_i2s_speaker() {
    // Configurar I2S
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SPEAKER_SERIAL_CLOCK,
        .ws_io_num = I2S_SPEAKER_LEFT_RIGHT_CLOCK,
        .data_out_num = I2S_SPEAKER_SERIAL_DATA,
        .data_in_num = I2S_PIN_NO_CHANGE
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

void play_audio(int16_t* audio_data, size_t audio_size) {
    size_t bytes_written;
    esp_err_t err = i2s_write(I2S_NUM_0, audio_data, audio_size * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    if (err != ESP_OK) {
        Serial.printf("Error al escribir datos I2S: %d\n", err);
    }
}

bool play_audio_from_sd(FS* sd, const char* filePath) {
  // Abrir el archivo de audio
  File audioFile = sd->open(filePath, FILE_READ);
  if (!audioFile) {
    Serial.println("Error al abrir el archivo de audio!");
    return false;
  }
  Serial.println("Archivo de audio abierto.");

  // Saltar el encabezado WAV (44 bytes)
  if (audioFile.size() > 44) {
    audioFile.seek(44);  // Saltar los primeros 44 bytes (encabezado WAV)
  } else {
    Serial.println("El archivo de audio es demasiado pequeño para ser un WAV válido.");
    audioFile.close();
    return false;
  }

  // Buffer para almacenar los datos de audio
  uint8_t audioBuffer[AUDIO_BUFFER_SIZE];

  // Reproducir el archivo de audio
  Serial.println("Reproduciendo audio...");
  size_t bytesRead;
  while ((bytesRead = audioFile.read(audioBuffer, AUDIO_BUFFER_SIZE)) > 0) {
    size_t bytesWritten;
    esp_err_t err = i2s_write(I2S_NUM_0, audioBuffer, bytesRead, &bytesWritten, portMAX_DELAY);
    if (err != ESP_OK) {
      Serial.printf("Error al escribir datos I2S: %d\n", err);
      break;
    }
  }

  // Cerrar el archivo de audio
  audioFile.close();
  Serial.println("Reproducción de audio finalizada.");

  return true;
}