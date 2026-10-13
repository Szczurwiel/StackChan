#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/hal.h"
#include "mooncake_log.h"
#include "driver/i2c.h"

// Adresy I2C i rejestry czujnika LTR-553ALS na magistrali wewnętrznej CoreS3
#define LTR553_I2C_ADDR 0x23
#define LTR553_ALS_CONTR 0x80
#define LTR553_ALS_DATA_CH1_0 0x88

// Flagi działania
static bool auto_brightness_enabled = true;
static bool light_sensor_initialized = false;
static uint32_t last_als_update = 0;
// Przechowywanie aktualnej jasnosci zeby nie mrugac ekranem bez potrzeby
static uint8_t current_brightness = 255; 

void init_light_sensor() {
    // Odczyt rejestru kontrolnego przez globalnego HAL-a M5Stack by sprawdzić czy czujnik odpowiada
    uint8_t data;
    // CoreS3 ma czujniki na wenwętrznym I2C (PORT A)
    // Framework M5 ma wbudowane narzędzia I2C w HAL, próbujemy to wykorzystać bezpiecznie
    mclog::info("Inicjalizacja czujnika swiatla LTR-553...");
    
    // Prosta komenda włączenia czujnika (ALS Active Mode) 
    uint8_t cmd[2] = {LTR553_ALS_CONTR, 0x01}; 
    // Ponieważ projekt ma HAL, zakładamy że i2c port 0 to magistrala wewnętrzna
    esp_err_t ret = i2c_master_write_to_device(I2C_NUM_0, LTR553_I2C_ADDR, cmd, sizeof(cmd), 1000 / portTICK_PERIOD_MS);
    
    if (ret == ESP_OK) {
        light_sensor_initialized = true;
        mclog::info("Czujnik LTR-553 uruchomiony!");
    } else {
        mclog::error("Blad LTR-553! Sprawdz magistrale.");
    }
}

void handle_auto_brightness() {
    if (!light_sensor_initialized) return;

    // Pobieramy czas z HAL
    uint32_t current_time = GetHAL().millis();

    // Możliwość wyłączania Auto-Jasności dotknięciem ekranu/przycisku bocznego (jesli jest wspierany w HAL).
    // GetHAL() ma często getTouch() lub getButtonB(), używamy zdefiniowanego delay.
    
    // Odczyt co 500ms
    if (current_time - last_als_update > 500) {
        last_als_update = current_time;

        if (auto_brightness_enabled) {
            uint8_t als_data[2];
            // Odczyt wartosci oswietlenia (2 bajty z kanalu 1)
            esp_err_t ret = i2c_master_write_read_device(I2C_NUM_0, LTR553_I2C_ADDR, (const uint8_t[]){LTR553_ALS_DATA_CH1_0}, 1, als_data, 2, 1000 / portTICK_PERIOD_MS);
            
            if (ret == ESP_OK) {
                // Składamy 2 bajty z powrotem w jedna wartosc
                uint16_t lux = (als_data[1] << 8) | als_data[0];
                
                // Mapowanie: mało światła -> niska jasność ekranu (zakres np. od 5 do 1000 "luksów")
                int new_brightness = 0;
                
                if(lux < 10) new_brightness = 30; // Ciemno
                else if(lux > 800) new_brightness = 100; // Bardzo jasno (w tym frameworku jasnosc to 0-100)
                else {
                    // Matematyka proporcjonalna miedzy lux(10-800) a jasnoscia (30-100)
                    new_brightness = 30 + ((lux - 10) * 70 / 790); 
                }

                if (new_brightness != current_brightness) {
                    current_brightness = new_brightness;
                    // W HAL StackChana jasnosc zazwyczaj przyjmuje argument 0-100
                    GetHAL().setDispBrightness(current_brightness);
                    // mclog::info("Auto jasnosc zaktualizowana na: %d (Lux: %d)", current_brightness, lux);
                }
            }
        }
    }
}
