#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "TuyaWifi.h"

using namespace std;

#define UART_ID uart0
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

#define UART_TX_PIN 0
#define UART_RX_PIN 1

const uint LED_PIN = 25;
const uint KEY_PIN = 3;

TuyaWifi my_device;

unsigned char pid[] = {"ma67l9sgmdyg3d2k"};
unsigned char mcu_ver[] = {"1.0.0"};

/* Current LED status */
unsigned char led_state = 0;

/* Data point define */
#define DPID_SWITCH 20

/* Stores all DPs and their types. PS: array[][0]:dpid, array[][1]:dp type. 
 *                                     dp type(TuyaDefs.h) : DP_TYPE_RAW, DP_TYPE_BOOL, DP_TYPE_VALUE, DP_TYPE_STRING, DP_TYPE_ENUM, DP_TYPE_BITMAP
*/
unsigned char dp_array[][2] =
{
    {DPID_SWITCH, DP_TYPE_BOOL},
};

/**
 * @description: DP download callback function.
 * @param {unsigned char} dpid
 * @param {const unsigned char} value
 * @param {unsigned short} length
 * @return {unsigned char}
 */
unsigned char dp_process(unsigned char dpid,const unsigned char value[], unsigned short length)
{
    switch(dpid) {
        case DPID_SWITCH:
        led_state = my_device.mcu_get_dp_download_data(dpid, value, length); /* Get the value of the down DP command */
        if (led_state) {
            //Turn on
            gpio_put(LED_PIN, 1);
        } else {
            //Turn off
            gpio_put(LED_PIN, 0);
        }
        //Status changes should be reported.
        my_device.mcu_dp_update(dpid, value, length);
        break;

        default:break;
    }
    return SUCCESS;
}

/**
 * @description: Upload all DP status of the current device.
 * @param {*}
 * @return {*}
 */
void dp_update_all(void)
{
    my_device.mcu_dp_update(DPID_SWITCH, led_state, 1);
}

// RX interrupt handler
void on_uart_rx() {
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);
        // Can we send it back?
        if (uart_is_writable(UART_ID)) {
            tuya_uart.uart_receive_input(ch);
        }
    }
}

void pico_uart_init(void)
{
    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 2400);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    int actual = uart_set_baudrate(UART_ID, BAUD_RATE);
    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);
}

void pico_gpio_init(void)
{
    /* LED initialization */
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    /* KEY init */
    gpio_init(KEY_PIN);
    gpio_set_dir(KEY_PIN, GPIO_IN);
    gpio_set_pulls(KEY_PIN, true, false);
}

int main()
{
    pico_uart_init();
    pico_gpio_init();

    my_device.init(pid, mcu_ver);
    //incoming all DPs and their types array, DP numbers
    my_device.set_dp_cmd_total(dp_array, 1);
    //register DP download processing callback function
    my_device.dp_process_func_register(dp_process);
    //register upload all DP callback function
    my_device.dp_update_all_func_register(dp_update_all);

    while (true) {
        if (gpio_get(KEY_PIN) == 0) {
            sleep_ms(500);
            if (gpio_get(KEY_PIN) == 0) {
                my_device.mcu_set_wifi_mode(SMART_CONFIG);
            }
        }

        my_device.uart_service();
        sleep_ms(100);
    }

    return 0;
}

