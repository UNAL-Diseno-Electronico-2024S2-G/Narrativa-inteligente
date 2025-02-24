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
#include "../i2s_utils.h"
#include "../gpio_pins.h"
#include <EEPROM.h>             // Para llevar el número de foto

const char *ssid = "EMILIO1234";
const char *password = "Angelita";
#define SERVER_URL "http://192.168.1.21:8888/uploadAudio"

#define PHOTO_SERVER_URL "http://192.168.1.21:8888/uploadPhoto"

void setup() {
    Serial.begin(115200);
    ets_printf("Serial inicializado a 115200\n");
  
    // Inicializar FastLED y prender LED en verde (estado inactivo)
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
    leds[0] = CRGB::Green;
    FastLED.show();
  
    // Configurar el pulsador con resistencia interna pull-up
    pinMode(buttonPin, INPUT_PULLUP);

    init_wifi();
}
  
void loop() {}