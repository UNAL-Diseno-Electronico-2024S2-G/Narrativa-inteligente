#include "../i2s_pins.h"
#include "ESP_I2S.h"

I2SClass* init_i2s() {
  // Crear una instancia dinámica del objeto I2S
  I2SClass* i2s = new I2SClass();

  Serial.println("Inicializando bus I2S...");

  // Configurar los pines para la entrada de audio
  i2s->setPins(I2S_SCK, I2S_WS, -1, I2S_SD);

  // Inicializar el bus I2S en modo estándar, 16 kHz, 32 bits, modo mono (canal izquierdo)
  if (!i2s->begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT)) {
    Serial.println("Failed to initialize I2S bus!");
    delete i2s; // Liberar memoria en caso de fallo
    return nullptr;
  }
  
  Serial.println("Bus I2S inicializado correctamente.");
  return i2s;
}