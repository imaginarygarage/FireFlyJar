/*********************************************************************************
 *
 *  leds.c
 *
 *  Created on: Aug 11, 2019
 *      Author: Tanner Leland
 *       Brief: Supports all LED functionality.
 *
 *       An analog switch is used to multiplex the DAC output to 8 different LED
 *       drivers. Output is selected through three switch select pins and a switch
 *       nEnable pin. When switch nEnable is high, all outputs enter a high
 *       impedance state. When it is low, only the output selected by the switch
 *       select pins enters a low impedance state.
 *
 ********************************************************************************/


/*--------------------------------------------------------------------------------
                                   INCLUDES
--------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stm32f3xx.h>

#include "system.h"
#include "gpio.h"
#include "leds.h"


/*--------------------------------------------------------------------------------
                                      TYPES
--------------------------------------------------------------------------------*/

enum analog_switch_pin_type
{
    ANALOG_SWITCH_SELECT_0,
    ANALOG_SWITCH_SELECT_1,
    ANALOG_SWITCH_SELECT_2,
    ANALOG_SWITCH_NENABLE,

    ANALOG_SWITCH_PIN_FIRST = ANALOG_SWITCH_SELECT_0,
    ANALOG_SWITCH_PIN_FINAL = ANALOG_SWITCH_NENABLE,
    ANALOG_SWITCH_PIN_COUNT = ANALOG_SWITCH_PIN_FINAL - ANALOG_SWITCH_PIN_FIRST + 1
};


/*--------------------------------------------------------------------------------
                               MEMORY_CONSTANTS
--------------------------------------------------------------------------------*/

static const gpio_type analog_switch_io[] =
{
    /*--------------------------------------------------------
    { Port, Pin }
    --------------------------------------------------------*/
    { GPIOA,   9 },     /* ANALOG_SWITCH_SELECT_0           */
    { GPIOA,  10 },     /* ANALOG_SWITCH_SELECT_1           */
    { GPIOA,  11 },     /* ANALOG_SWITCH_SELECT_2           */
    { GPIOA,  12 },     /* ANALOG_SWITCH_NENABLE            */
};


/*--------------------------------------------------------------------------------
                               GLOBAL VARIABLES
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
                               STATIC VARIABLES
--------------------------------------------------------------------------------*/

volatile static int32_t s_led_brightness[ LED_COUNT ];


/*--------------------------------------------------------------------------------
                                  PROCEDURES
--------------------------------------------------------------------------------*/

static void dac_enable_output
    (
    boolean enable
    );


static void dac_init
    (
    void
    );

static void dac_set_led
    (
    led_type    led_id
    );

static void dac_set_output
    (
    uint32_t    dac_val
    );

static void led_periodic_callback
    (
    void
    );

static void led_update_brightness
    (
    led_type    led_id,
    uint32_t    led_brightness
    );


/*************************************************************************
 *
 *  Procedure:
 *      led_init
 *
 *  Description:
 *      Initialize LED functionality.
 *
 ************************************************************************/
void led_init
    (
    void
    )
{
    /*--------------------------------------------------------
    Initialize the multiplexed output of the digital to analog
    converter (DAC).
    --------------------------------------------------------*/
    dac_init();

    /*--------------------------------------------------------
    Initialize LED brightness values to 0
    --------------------------------------------------------*/
    clear_array( s_led_brightness );

    /*--------------------------------------------------------
    Register periodic callback function
    --------------------------------------------------------*/
    system_add_task( led_periodic_callback, 1 );

} /* led_init */


/*************************************************************************
 *
 *  Procedure:
 *      led_set_brightness
 *
 *  Description:
 *      Set the brightness value that will be applied during periodic
 *      updates.
 *
 ************************************************************************/
void led_set_brightness
    (
    led_type    led_id,
    uint32_t    led_brightness
    )
{
    /*--------------------------------------------------------
    Set the brightness value if led_id is valid
    --------------------------------------------------------*/
    if( led_id < LED_COUNT )
    {
        s_led_brightness[ led_id ] = led_brightness;
    }

} /* led_set_brightness */


/*************************************************************************
 *
 *  Procedure:
 *      led_periodic_callback
 *
 *  Description:
 *      Cycles through the LEDs, updating the brightness of one for every
 *      call. Brightness is controlled by connecting the DAC output to
 *      a driver's input through an analog switch. Each driver input holds
 *      the voltage level with small capacitor while the other 7 LEDs are
 *      adjusted.
 *
 ************************************************************************/
static void led_periodic_callback
    (
    void
    )
{
    /*--------------------------------------------------------
    Local static variables
    --------------------------------------------------------*/
    static led_type     led_id;

    /*--------------------------------------------------------
    Update LED brightness
    --------------------------------------------------------*/
    led_update_brightness( led_id, s_led_brightness[ led_id ] );

    /*--------------------------------------------------------
    Increment LED
    --------------------------------------------------------*/
    led_id = (led_id + 1) % LED_COUNT;

} /* led_periodic_callback */


/*************************************************************************
 *
 *  Procedure:
 *      led_update_brightness
 *
 *  Description:
 *      Set the output of the DAC and routes it to the appropriate driver.
 *
 ************************************************************************/
static void led_update_brightness
    (
    led_type    led_id,
    uint32_t    led_brightness
    )
{
    /*--------------------------------------------------------
    Disable analog switch output during voltage transition
    --------------------------------------------------------*/
    dac_enable_output( FALSE );

    /*--------------------------------------------------------
    Update DAC output
    --------------------------------------------------------*/
    dac_set_output( led_brightness );

    /*--------------------------------------------------------
    Select the switch connected to the desired LED driver.
    --------------------------------------------------------*/
    dac_set_led( led_id );

    /*--------------------------------------------------------
    Enable analog switch output to LED driver
    --------------------------------------------------------*/
    dac_enable_output( TRUE );

} /* led_update_brightness */


/*************************************************************************
 *
 *  Procedure:
 *      dac_init
 *
 *  Description:
 *      Initialize DAC functionality.
 *
 ************************************************************************/
static void dac_init
    (
    void
    )
{
    /*--------------------------------------------------------
    First, configure the pins that control the analog switch
    responsible for demultiplexing the time-division
    multiplexed DAC signal. Output from the analog switch is
    disabled by default.
    --------------------------------------------------------*/
    for( int i = ANALOG_SWITCH_PIN_FIRST; i <= ANALOG_SWITCH_PIN_FINAL; i++ )
    {
        gpio_cfg_output( &analog_switch_io[ i ] );
    }

    /*--------------------------------------------------------
    Enable clock to DAC peripheral
    --------------------------------------------------------*/
    RCC->APB1ENR |= RCC_APB1ENR_DAC1EN;

    /*--------------------------------------------------------
    Disable the DAC output buffer to enable rail-to-rail
    output. An external buffer with rail-to-rail capability is
    used instead.
    --------------------------------------------------------*/
    DAC->CR |= DAC_CR_BOFF1;

    /*--------------------------------------------------------
    Enable DAC peripheral
    --------------------------------------------------------*/
    DAC->CR |= DAC_CR_EN1;

    /*--------------------------------------------------------
    Set initial DAC output value to 0V
    --------------------------------------------------------*/
    dac_set_output( 0 );

} /* dac_init */


/*************************************************************************
 *
 *  Procedure:
 *      dac_enable_output
 *
 *  Description:
 *      Enables/disables the analog switch output that acts as a MUX for
 *      the DAC output to the LED drivers.
 *
 ************************************************************************/
static void dac_enable_output
    (
    boolean enable
    )
{
    /*--------------------------------------------------------
    Local variables
    --------------------------------------------------------*/
    const gpio_type * nenable;
    gpio_state_type   state;

    /*--------------------------------------------------------
    Get the GPIO controlling the analog switch's NENABLE input
    and the desired state.
    --------------------------------------------------------*/
    nenable = &analog_switch_io[ ANALOG_SWITCH_NENABLE ];
    state = enable ? GPIO_STATE_LOW : GPIO_STATE_HIGH;

    /*--------------------------------------------------------
    Update the state of the NENABLE input in order to enable
    or disable the analog switch output
    --------------------------------------------------------*/
    gpio_output_set( nenable, state );

} /* dac_enable_output */


/*************************************************************************
 *
 *  Procedure:
 *      dac_set_led
 *
 *  Description:
 *      Set LED driver that the DAC is routed to.
 *
 ************************************************************************/
static void dac_set_led
    (
    led_type    led_id
    )
{
    /*--------------------------------------------------------
    Local variables
    --------------------------------------------------------*/
    const gpio_type * select;
    boolean           state;

    /*--------------------------------------------------------
    Get the GPIO controlling the analog switch's output
    selection, as well as the desired state for each.
    --------------------------------------------------------*/
    for( int i = ANALOG_SWITCH_SELECT_0; i <= ANALOG_SWITCH_SELECT_2; i++ )
    {
        select = &analog_switch_io[ i ];
        state  = (led_id & (1 << (i - ANALOG_SWITCH_SELECT_0))) ? GPIO_STATE_HIGH : GPIO_STATE_LOW;
        gpio_output_set( select, state );
    }

} /* dac_set_led */


/*************************************************************************
 *
 *  Procedure:
 *      dac_set_output
 *
 *  Description:
 *      Set the 12-bit output level of the DAC.
 *
 ************************************************************************/
static void dac_set_output
    (
    uint32_t    dac_val
    )
{
    /*--------------------------------------------------------
    Update DAC output
    --------------------------------------------------------*/
    DAC->DHR12R1 = dac_val & 0x00000FFF;

} /* dac_set_output */
