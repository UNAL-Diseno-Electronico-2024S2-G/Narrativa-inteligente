#include "../wifi_utils.h"
#include "../sd_card_utils.h"
#include "../gpio_pins.h"

#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <EEPROM.h>   
#include <FastLED.h>         // read and write from flash memory

CRGB leds[NUM_LEDS];

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
  
}
  
void loop() {
    delay(10);
}