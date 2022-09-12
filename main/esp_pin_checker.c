/* Check pins of ESP using internal week pull up and pull down resistors

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
//#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

/* Count of all pads of ESP chip. 
Note that some pads are not exposed outside of package as physical GPIOs
*/
#define ESP_PAD_COUNT    (CONFIG_SOC_GPIO_PIN_COUNT)
#define SKIP_PADS        (CONFIG_SKIP_PADS)

/* Set state of GPIO input by enabling / disabling 
   internal weak pull up (WPU) / pull down (WPD) resistors
*/
static void gpio_set_state(gpio_num_t gpio_num, int gpio_state)
{
    if (gpio_state == 0) {
        gpio_pullup_dis(gpio_num);
        gpio_pulldown_en(gpio_num);
    } else if (gpio_state == 1) {
        gpio_pulldown_dis(gpio_num);
        gpio_pullup_en(gpio_num);
    } else {
        gpio_pulldown_dis(gpio_num);
        gpio_pullup_dis(gpio_num);
    }
}


/* Check if GPIO input state corresponds with setting
   of internal weak pull up (WPU) / pull down (WPD) resistors
   Note: It is assumed that GPIO is not connected to any circuit 
   that may influence state of WPU / WPD
*/
static bool gpio_check_wpd_wpu(gpio_num_t gpio_num)
{
    gpio_set_direction(gpio_num, GPIO_MODE_DEF_INPUT);

    int gpio_state = 0;
    bool result = true;
    gpio_set_state(gpio_num, gpio_state);
    if (gpio_get_level(gpio_num) != gpio_state) {
        printf("GPIO %2d has unexpected state %d\n", gpio_num, gpio_state);
        result &= false;
    } else {
        result &= true;
    }
    gpio_state = !gpio_state;
    gpio_set_state(gpio_num, gpio_state);
    if (gpio_get_level(gpio_num) != gpio_state) {
        printf("GPIO %2d has unexpected state %d\n", gpio_num, gpio_state);
        result &= false;
    } else {
        result &= true;
    }
    gpio_set_state(gpio_num, -1); // clear WPD/WPU bits
    return result;
}


/* Convert pad numbers from string into integers 
and save in an integer array for easier future access.
We do not know upfront the count of pads
therfore the integer array is dynamically allocated.
*/
int *get_pads(const char *pad_list, int *pad_count) {

    printf("Pads to skip: %s\n", pad_list);
    int *pad_array = NULL;
    const char *p_start = pad_list;
    while(p_start < pad_list + strlen(pad_list)) {
        char* p_end;
        long int value = strtol(p_start, &p_end, 10);
        if(value == 0L && p_start == p_end)
            break;
    
        pad_array = (int *) realloc(pad_array, sizeof(int));
        pad_array[*pad_count] = (int) value;
        *pad_count = *pad_count + 1;
        p_start = p_end;
    }
    return pad_array;
}


void app_main(void)
{
    int *skip_pad_array;
    int skip_pad_count = 0;

    skip_pad_array = get_pads(SKIP_PADS, &skip_pad_count);

    while (1) { 
        printf("Checking %d pads out of %d ...\n", ESP_PAD_COUNT - skip_pad_count, ESP_PAD_COUNT);
        printf("Pads to skip: ");
        for (int i=0; i<skip_pad_count; i++){
            printf("%d ", skip_pad_array[i]);
        }
        printf("\n");
        bool skip_check;
        for (int i=0; i<ESP_PAD_COUNT; i++) {
            skip_check = false;
            for (int j=0; j<skip_pad_count; j++) {
                if(i == skip_pad_array[j]) {
                    skip_check = true;
                    break;
                }
            }
            if (!skip_check) {
                bool check_passed = gpio_check_wpd_wpu(i);
                printf("GPIO %2d check %s", i, (check_passed ? "passed\n" : "FAILED!\n"));
            }
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
