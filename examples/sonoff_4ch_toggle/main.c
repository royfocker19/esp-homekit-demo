/*
 * Example of using esp-homekit library to control
 * a simple ESP12F using HomeKit.
 * The esp-wifi-config library is also used in this
 * example. This means you don't have to specify
 * your network's SSID and password before building.
 *
 * In order to flash the ESP12F 4CH you will have to
 * have a 3,3v (logic level) FTDI adapter.
 *
 * To flash this example connect 3,3v, TX, RX, GND
 * in this order, beginning in the (square) pin header
 * at the end of the board.
 *
 * The physical button on the board connects to GPIO0 used to flash board!
 *
 * WARNING: Do not connect the board to AC while it's
 * connected to the FTDI adapter! This may fry your
 * computer and ESP12F.
 *
 * Changes:
 * Modified example Sonoff_Dual_toggle to address Four channels
 * Implemented as four linked homekit characteristics (4x switch)
 * Inabled push-button (GPIO0) as input and use to change state
 * implemented short/medium/long button press events to
 * toggle triac1, toggle triac2, toggle triac3, toggle triac4 and reset wifi respectivly
 * implemented toggle on "Button 0" (GPIO4), "Button 1" (GPIO5), "Button 2" (GPIO9) and "Button 3" (GPIO10)
 */

#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

#include "button.h"
#include "toggle.h"

// The GPIO pin that is connected to TRIAC#1 on the board.
const int triac_gpio_1 = 13;
// The GPIO pin that is connected to TRIAC#2 on the board.
const int triac_gpio_2 = 12;
// The GPIO pin that is connected to TRIAC#1 on the board.
const int triac_gpio_3 = 14;
// The GPIO pin that is connected to TRIAC#2 on the board.
const int triac_gpio_4 = 16;

// The GPIO pin that is connected to the LED on the ESP12F.
const int led_gpio = 2;

// The GPIO pin that is connected to the button on the Board.
const int button_gpio = 0;

// The GPIO pin that is connected to the header on the board(external switch).
const int toggle_gpio_1 = 4;
const int toggle_gpio_2 = 5;
const int toggle_gpio_3 = 3;
const int toggle_gpio_4 = 1;

void lightbulb_on_1_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context);
void lightbulb_on_2_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context);
void lightbulb_on_3_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context);
void lightbulb_on_4_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context);

void button_callback(uint8_t gpio, button_event_t event);

void toggle_callback_1(uint8_t gpio);
void toggle_callback_2(uint8_t gpio);
void toggle_callback_3(uint8_t gpio);
void toggle_callback_4(uint8_t gpio);


void triac_write_1(bool on) {
    gpio_write(triac_gpio_1, on ? 1 : 0);
}

void triac_write_2(bool on) {
    gpio_write(triac_gpio_2, on ? 1 : 0);
}

void triac_write_3(bool on) {
    gpio_write(triac_gpio_3, on ? 1 : 0);
}

void triac_write_4(bool on) {
    gpio_write(triac_gpio_4, on ? 1 : 0);
}

void led_write(bool on) {
    gpio_write(led_gpio, on ? 0 : 1);
}


void reset_configuration_task() {
    //Flash LED first before we start the reset
    for (int i=0; i<3; i++) {
        led_write(true);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        led_write(false);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    printf("Resetting Wifi due to button long at GPIO %2d\n", button_gpio);
    wifi_config_reset();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("Resetting HomeKit Config\n");
    homekit_server_reset();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("Restarting\n");
    sdk_system_restart();
    vTaskDelete(NULL);
}

void reset_configuration() {
    printf("Resetting ESP12F configuration\n");
    xTaskCreate(reset_configuration_task, "Reset configuration", 256, NULL, 2, NULL);
}

homekit_characteristic_t lightbulb_on_1 = HOMEKIT_CHARACTERISTIC_(
    ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(lightbulb_on_1_callback)
);

homekit_characteristic_t lightbulb_on_2 = HOMEKIT_CHARACTERISTIC_(
    ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(lightbulb_on_2_callback)
);

homekit_characteristic_t lightbulb_on_3 = HOMEKIT_CHARACTERISTIC_(
    ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(lightbulb_on_3_callback)
);

homekit_characteristic_t lightbulb_on_4 = HOMEKIT_CHARACTERISTIC_(
    ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(lightbulb_on_4_callback)
);


void gpio_init() {
    gpio_enable(led_gpio, GPIO_OUTPUT);
    led_write(false);
    
    gpio_enable(triac_gpio_1, GPIO_OUTPUT);
    triac_write_1(lightbulb_on_1.value.bool_value);
    
    gpio_enable(triac_gpio_2, GPIO_OUTPUT);
    triac_write_2(lightbulb_on_2.value.bool_value);
    
    gpio_enable(triac_gpio_3, GPIO_OUTPUT);
    triac_write_3(lightbulb_on_3.value.bool_value);
    
    gpio_enable(triac_gpio_4, GPIO_OUTPUT);
    triac_write_4(lightbulb_on_4.value.bool_value);
    
    gpio_enable(button_gpio,   GPIO_INPUT);
    gpio_enable(toggle_gpio_1, GPIO_INPUT);
    gpio_enable(toggle_gpio_2, GPIO_INPUT);
    gpio_enable(toggle_gpio_3, GPIO_INPUT);
    gpio_enable(toggle_gpio_4, GPIO_INPUT);
}

void lightbulb_on_1_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
    triac_write_1(lightbulb_on_1.value.bool_value);
}

void lightbulb_on_2_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
    triac_write_2(lightbulb_on_2.value.bool_value);
}

void lightbulb_on_3_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
    triac_write_3(lightbulb_on_3.value.bool_value);
}

void lightbulb_on_4_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
    triac_write_4(lightbulb_on_4.value.bool_value);
}


void button_callback(uint8_t gpio, button_event_t event) {
    switch (event) {
        case button_event_single_press:
            printf("Toggling triac 1 due to button short  at GPIO %2d\n", gpio);
            lightbulb_on_1.value.bool_value = !lightbulb_on_1.value.bool_value;
            triac_write_1(lightbulb_on_1.value.bool_value);
            homekit_characteristic_notify(&lightbulb_on_1, lightbulb_on_1.value);
            break;
        case button_event_medium2_press:
            printf("Toggling triac 2 due to button medium at GPIO %2d\n", gpio);
            lightbulb_on_2.value.bool_value = !lightbulb_on_2.value.bool_value;
            triac_write_2(lightbulb_on_2.value.bool_value);
            homekit_characteristic_notify(&lightbulb_on_2, lightbulb_on_2.value);
            break;
        case button_event_medium3_press:
            printf("Toggling triac 3 due to button medium at GPIO %2d\n", gpio);
            lightbulb_on_3.value.bool_value = !lightbulb_on_3.value.bool_value;
            triac_write_3(lightbulb_on_3.value.bool_value);
            homekit_characteristic_notify(&lightbulb_on_3, lightbulb_on_3.value);
            break;
        case button_event_medium4_press:
            printf("Toggling triac 4 due to button medium at GPIO %2d\n", gpio);
            lightbulb_on_4.value.bool_value = !lightbulb_on_4.value.bool_value;
            triac_write_4(lightbulb_on_4.value.bool_value);
            homekit_characteristic_notify(&lightbulb_on_4, lightbulb_on_4.value);
            break;
        case button_event_long_press:
            reset_configuration();
            break;
        default:
            printf("Unknown button event: %d\n", event);
    }
}

void toggle_callback_1(uint8_t gpio) {
    printf("Toggling TRIAC 1 due to switch at GPIO %2d\n", gpio);
    lightbulb_on_1.value.bool_value = !lightbulb_on_1.value.bool_value;
    triac_write_1(lightbulb_on_1.value.bool_value);
    homekit_characteristic_notify(&lightbulb_on_1, lightbulb_on_1.value);
}

void toggle_callback_2(uint8_t gpio) {
    printf("Toggling TRIAC 2 due to switch at GPIO %2d\n", gpio);
    lightbulb_on_2.value.bool_value = !lightbulb_on_2.value.bool_value;
    triac_write_2(lightbulb_on_2.value.bool_value);
    homekit_characteristic_notify(&lightbulb_on_2, lightbulb_on_2.value);
}

void toggle_callback_3(uint8_t gpio) {
    printf("Toggling TRIAC 3 due to switch at GPIO %2d\n", gpio);
    lightbulb_on_3.value.bool_value = !lightbulb_on_3.value.bool_value;
    triac_write_3(lightbulb_on_3.value.bool_value);
    homekit_characteristic_notify(&lightbulb_on_3, lightbulb_on_3.value);
}

void toggle_callback_4(uint8_t gpio) {
    printf("Toggling TRIAC 4 due to switch at GPIO %2d\n", gpio);
    lightbulb_on_4.value.bool_value = !lightbulb_on_4.value.bool_value;
    triac_write_4(lightbulb_on_4.value.bool_value);
    homekit_characteristic_notify(&lightbulb_on_4, lightbulb_on_4.value);
}

void light_identify_task(void *_args) {
    // We identify the ESP by Flashing it's LED.
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            led_write(true);
            vTaskDelay(200 / portTICK_PERIOD_MS);
            led_write(false);
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    led_write(false);
    vTaskDelete(NULL);
}

void light_identify(homekit_value_t _value) {
    printf("Light identify\n");
    xTaskCreate(light_identify_task, "Light identify", 128, NULL, 2, NULL);
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "ESP12F 4CH");

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(IDENTIFY, light_identify),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "LKAI"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP 4CH"),
            &name,
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "mps"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1"),
            HOMEKIT_CHARACTERISTIC(HARDWARE_REVISION, "V2"),
            NULL
        }),
        
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
	    HOMEKIT_CHARACTERISTIC(NAME, "Light 1"),
	    &lightbulb_on_1,
            NULL
        }),
        NULL
    }),
    
    HOMEKIT_ACCESSORY(.id=2, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(IDENTIFY, light_identify),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "LKAI"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP 4CH"),
            HOMEKIT_CHARACTERISTIC(NAME, "ESP 4CH2"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "mps2"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1"),
            HOMEKIT_CHARACTERISTIC(HARDWARE_REVISION, "V2"),
            NULL
        }),
        
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Light 2"),
            &lightbulb_on_2,
            NULL
        }),
        NULL
    }),
    
    HOMEKIT_ACCESSORY(.id=3, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(IDENTIFY, light_identify),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "LKAI"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP 4CH"),
            HOMEKIT_CHARACTERISTIC(NAME, "ESP 4CH3"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "mps3"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1"),
            HOMEKIT_CHARACTERISTIC(HARDWARE_REVISION, "V2"),
            NULL
        }),
        
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Light 3"),
            &lightbulb_on_3,
            NULL
        }),
        NULL
    }),
    
    HOMEKIT_ACCESSORY(.id=4, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(IDENTIFY, light_identify),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "LKAI"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP 4CH"),
            HOMEKIT_CHARACTERISTIC(NAME, "ESP 4CH4"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "mps4"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1"),
            HOMEKIT_CHARACTERISTIC(HARDWARE_REVISION, "V2"),
            NULL
        }),
        
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Light 4"),
            &lightbulb_on_4,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "201-01-985"
//    .password = "190-11-978"    //changed tobe valid
//    .password = "111-11-111"    //default easy
};

void on_wifi_ready() {
    homekit_server_init(&config);
}

void create_accessory_name() {
    uint8_t macaddr[6];
    sdk_wifi_get_macaddr(STATION_IF, macaddr);
    
    int name_len = snprintf(NULL, 0, "ESP12F 4CH %02X:%02X:%02X",
            macaddr[3], macaddr[4], macaddr[5]);
    char *name_value = malloc(name_len+1);
    snprintf(name_value, name_len+1, "ESP12F 4CH %02X:%02X:%02X",
            macaddr[3], macaddr[4], macaddr[5]);
    
    name.value = HOMEKIT_STRING(name_value);
}


void user_init(void) {
    uart_set_baud(0, 115200);
    create_accessory_name();
    wifi_config_init("ESP12F 4CH", NULL, on_wifi_ready);
    gpio_init();

// 4000 = 4s long press => 15000/3 = 1s medium press
    if (button_create(button_gpio, 0, 20000, button_callback)) {
        printf("Failed to initialize button\n");
    }
    
    if (toggle_create(toggle_gpio_1, toggle_callback_1)) {
        printf("Failed to initialize toggle 1 \n");
    }

    if (toggle_create(toggle_gpio_2, toggle_callback_2)) {
        printf("Failed to initialize toggle 2 \n");
    }
    
    if (toggle_create(toggle_gpio_3, toggle_callback_3)) {
        printf("Failed to initialize toggle 3 \n");
    }
    
    if (toggle_create(toggle_gpio_4, toggle_callback_4)) {
        printf("Failed to initialize toggle 4 \n");
    }
}
