#pragma once
#include <M5Unified.h>
#include <LTR5XX.h> 

LTR5XX DistanceSensor;
bool light_sensor_initialized = false;
uint32_t last_als_update = 0;
bool auto_brightness_enabled = true; // Domyslnie wlaczona automatyczna jasnosc

void init_light_sensor() {
    Ltr5xx_Init_Basic_Para para = LTR5XX_BASE_PARA_CONFIG_DEFAULT;
    if (DistanceSensor.begin(&para)) {
        Serial.println("LTR-553ALS: Blad inicjalizacji.");
    } else {
        Serial.println("LTR-553ALS: Zainicjowano poprawnie!");
        DistanceSensor.setAlsMode(LTR5XX_ALS_ACTIVE_MODE);
        light_sensor_initialized = true;
    }
}

void handle_auto_brightness() {
    if (!light_sensor_initialized) return;

    // Przelaczanie trybu automatycznego srodkowym przyciskiem dotykowym (BtnB) pod ekranem
    if (M5.BtnB.wasPressed()) {
        auto_brightness_enabled = !auto_brightness_enabled;
        if (!auto_brightness_enabled) {
            Serial.println("Auto-jasnosc: WYLACZONE");
        } else {
            Serial.println("Auto-jasnosc: WLACZONE");
        }
    }

    if (auto_brightness_enabled) {
        // Aktualizacja jasnosci co 500ms, zeby nie zacinac ruchu oczu i serw robota
        if (millis() - last_als_update > 500) { 
            last_als_update = millis();
            uint16_t als_value = DistanceSensor.getAlsValue();
            
            // Czujnik zamkniety za szklem CoreS3 odbiera mniej swiatla, mapujemy na ekran (0-255)
            int new_brightness = map(als_value, 5, 800, 30, 255); 
            new_brightness = constrain(new_brightness, 30, 255); // Zabezpieczenie minimum 30, by nie zgasl w nocy!
            
            M5.Display.setBrightness(new_brightness);
        }
    }
}
