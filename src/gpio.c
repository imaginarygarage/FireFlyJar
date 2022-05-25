/*********************************************************************************
 *
 *  gpio.c
 *
 *  Created on: Aug 11, 2019
 *      Author: Tanner Leland
 *       Brief: The absolute minimum pin control necessary for this project.
 *
 ********************************************************************************/


/*--------------------------------------------------------------------------------
                                   INCLUDES
--------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stm32f3xx.h>

#include "system.h"
#include "gpio.h"


/*--------------------------------------------------------------------------------
                                LITERAL CONSTANTS
--------------------------------------------------------------------------------*/

#define GPIO_PIN_MAX    ( 15 )


/*--------------------------------------------------------------------------------
                                      TYPES
--------------------------------------------------------------------------------*/

typedef enum
{
    MODE_INPUT,
    MODE_OUTPUT,
    MODE_ALTERNATE_FUNCTION,
    MODE_ANALOG
} mode_type;

typedef enum
{
    OUTPUT_SPEED_LOW,
    OUTPUT_SPEED_MEDIUM,
    OUTPUT_SPEED_HIGH
} output_speed_type;

typedef enum
{
    OUTPUT_TYPE_PUSH_PULL,
    OUTPUT_TYPE_OPEN_DRAIN
} output_type;

typedef enum
{
    PULL_RESISTOR_NONE,
    PULL_RESISTOR_UP,
    PULL_RESISTOR_DOWN
} pull_resistor_type;


/*--------------------------------------------------------------------------------
                               GLOBAL VARIABLES
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
                               STATIC VARIABLES
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
                                  PROCEDURES
--------------------------------------------------------------------------------*/

static void configure_mode
    (
    const gpio_type   * gpio,
    mode_type           cfg
    );

static void configure_output_level
    (
    const gpio_type   * gpio,
    gpio_state_type     cfg
    );

static void configure_output_speed
    (
    const gpio_type   * gpio,
    output_speed_type   cfg
    );

static void configure_output_type
    (
    const gpio_type   * gpio,
    output_type         cfg
    );

static void configure_pull_resistors
    (
    const gpio_type   * gpio,
    pull_resistor_type  cfg
    );

static void enable_port
    (
    const gpio_type   * gpio
    );


/*************************************************************************
 *
 *  Procedure:
 *      gpio_cfg_input
 *
 *  Description:
 *      Configures given pin as a high impedence input.
 *
 ************************************************************************/
void gpio_cfg_input
    (
    const gpio_type   * gpio
    )
{
    /*--------------------------------------------------------
    Validate input
    --------------------------------------------------------*/
    if( gpio == NULL
     || gpio->port == NULL
     || gpio->pin > GPIO_PIN_MAX )
    {
        return;
    }

    /*--------------------------------------------------------
    Ensure the port is enabled
    --------------------------------------------------------*/
    enable_port( gpio );

    /*--------------------------------------------------------
    Set pin mode to output
    --------------------------------------------------------*/
    configure_mode( gpio, MODE_INPUT );

    /*--------------------------------------------------------
    Disable pull resistors
    --------------------------------------------------------*/
    configure_pull_resistors( gpio, PULL_RESISTOR_NONE );

} /* gpio_cfg_input */


/*************************************************************************
 *
 *  Procedure:
 *      gpio_cfg_output
 *
 *  Description:
 *      Configures given pin as a push-pull output, driven low
 *
 ************************************************************************/
void gpio_cfg_output
    (
    const gpio_type   * gpio
    )
{
    /*--------------------------------------------------------
    Validate input
    --------------------------------------------------------*/
    if( gpio == NULL
     || gpio->port == NULL
     || gpio->pin > GPIO_PIN_MAX )
    {
        return;
    }

    /*--------------------------------------------------------
    Ensure the port is enabled
    --------------------------------------------------------*/
    enable_port( gpio );

    /*--------------------------------------------------------
    Disable pull resistors
    --------------------------------------------------------*/
    configure_pull_resistors( gpio, PULL_RESISTOR_NONE );

    /*--------------------------------------------------------
    Set output speed to fast
    --------------------------------------------------------*/
    configure_output_speed( gpio, OUTPUT_SPEED_HIGH );

    /*--------------------------------------------------------
    Set output state to low
    --------------------------------------------------------*/
    configure_output_level( gpio, GPIO_STATE_LOW );

    /*--------------------------------------------------------
    Set output type to push-pull
    --------------------------------------------------------*/
    configure_output_type( gpio, OUTPUT_TYPE_PUSH_PULL );

    /*--------------------------------------------------------
    Set pin mode to output
    --------------------------------------------------------*/
    configure_mode( gpio, MODE_OUTPUT );

} /* gpio_cfg_output */


/*************************************************************************
 *
 *  Procedure:
 *      gpio_input_read
 *
 *  Description:
 *      Read the state of a given input pin.
 *
 ************************************************************************/
gpio_state_type gpio_input_read
    (
    const gpio_type   * gpio
    )
{
    /*--------------------------------------------------------
    Validate input
    --------------------------------------------------------*/
    if( gpio == NULL
     || gpio->port == NULL
     || gpio->pin > GPIO_PIN_MAX )
    {
        return( GPIO_STATE_LOW );
    }

    /*--------------------------------------------------------
    Check the state of the pin
    --------------------------------------------------------*/
    if( gpio->port->IDR & (1 << gpio->pin) )
    {
        return( GPIO_STATE_HIGH );
    }
    else
    {
        return( GPIO_STATE_LOW );
    }

} /* gpio_input_read */


/*************************************************************************
 *
 *  Procedure:
 *      gpio_output_set
 *
 *  Description:
 *      Drive a specific output pin high or low.
 *
 ************************************************************************/
void gpio_output_set
    (
    const gpio_type   * gpio,
    gpio_state_type     state
    )
{
    /*--------------------------------------------------------
    Validate input
    --------------------------------------------------------*/
    if( gpio == NULL
     || gpio->port == NULL
     || gpio->pin > GPIO_PIN_MAX )
    {
        return;
    }

    /*--------------------------------------------------------
    Set output state
    --------------------------------------------------------*/
    configure_output_level( gpio, state );

} /* gpio_output_set() */


/*************************************************************************
 *
 *  Procedure:
 *      configure_mode
 *
 *  Description:
 *      Configure the mode of the given pin.
 *
 ************************************************************************/
static void configure_mode
    (
    const gpio_type   * gpio,
    mode_type           cfg
    )
{
    /*--------------------------------------------------------
    Local variables
    --------------------------------------------------------*/
    uint32_t        pin;
    GPIO_TypeDef  * port;

    /*--------------------------------------------------------
    Initialize Variables
    --------------------------------------------------------*/
    pin  = gpio->pin;
    port = gpio->port;

    /*--------------------------------------------------------
    Configure the output level of the given pin.
    --------------------------------------------------------*/
    switch( cfg )
    {
        case MODE_OUTPUT:
            port->MODER &= ~(3 << (pin * 2));
            port->MODER |= 1 << (pin * 2);
            break;

        case MODE_ALTERNATE_FUNCTION:
            port->MODER &= ~(3 << (pin * 2));
            port->MODER |= 2 << (pin * 2);
            break;

        case MODE_ANALOG:
            port->MODER |= 3 << (pin * 2);
            break;

        case MODE_INPUT:
        default:
            port->MODER &= ~(3 << (pin * 2));
            break;
    }

} /* configure_mode */


/*************************************************************************
 *
 *  Procedure:
 *      configure_output_level
 *
 *  Description:
 *      Configure the output level of the given pin.
 *
 ************************************************************************/
static void configure_output_level
    (
    const gpio_type   * gpio,
    gpio_state_type     cfg
    )
{
    /*--------------------------------------------------------
    Local variables
    --------------------------------------------------------*/
    uint32_t        pin;
    GPIO_TypeDef  * port;

    /*--------------------------------------------------------
    Initialize Variables
    --------------------------------------------------------*/
    pin  = gpio->pin;
    port = gpio->port;

    /*--------------------------------------------------------
    Configure the output level of the given pin.
    --------------------------------------------------------*/
    switch( cfg )
    {
        case GPIO_STATE_HIGH:
            port->BSRRL = 1 << pin;
            break;

        case GPIO_STATE_LOW:
        default:
            port->BSRRH = 1 << pin;
            break;
    }

} /* configure_output_level */



/*************************************************************************
 *
 *  Procedure:
 *      configure_output_speed
 *
 *  Description:
 *      Configure the output speed of the given pin.
 *
 ************************************************************************/
static void configure_output_speed
    (
    const gpio_type   * gpio,
    output_speed_type   cfg
    )
{
    /*--------------------------------------------------------
    Local variables
    --------------------------------------------------------*/
    uint32_t        pin;
    GPIO_TypeDef  * port;

    /*--------------------------------------------------------
    Initialize Variables
    --------------------------------------------------------*/
    pin  = gpio->pin;
    port = gpio->port;

    /*--------------------------------------------------------
    Configure the output speed of the given pin.
    --------------------------------------------------------*/
    switch( cfg )
    {
        case OUTPUT_SPEED_LOW:
            port->OSPEEDR &= ~(3 << (pin * 2));
            break;

        case OUTPUT_SPEED_MEDIUM:
            port->OSPEEDR &= ~(3 << (pin * 2));
            port->OSPEEDR |= 1 << (pin * 2);
            break;

        case OUTPUT_SPEED_HIGH:
        default:
            port->OSPEEDR |= 3 << (pin * 2);
            break;
    }

} /* configure_output_speed */


/*************************************************************************
 *
 *  Procedure:
 *      configure_output_type
 *
 *  Description:
 *      Configure the type of output for the given pin.
 *
 ************************************************************************/
static void configure_output_type
    (
    const gpio_type   * gpio,
    output_type         cfg
    )
{
    /*--------------------------------------------------------
    Local variables
    --------------------------------------------------------*/
    uint32_t        pin;
    GPIO_TypeDef  * port;

    /*--------------------------------------------------------
    Initialize Variables
    --------------------------------------------------------*/
    pin  = gpio->pin;
    port = gpio->port;

    /*--------------------------------------------------------
    Configure the output level of the given pin.
    --------------------------------------------------------*/
    switch( cfg )
    {
        case OUTPUT_TYPE_OPEN_DRAIN:
            port->OTYPER |= 1 << pin;
            break;

        case OUTPUT_TYPE_PUSH_PULL:
        default:
            port->OTYPER &= ~(1 << pin);
            break;
    }

} /* configure_output_type */


/*************************************************************************
 *
 *  Procedure:
 *      configure_pull_resistors
 *
 *  Description:
 *      Configure the pull up and pull down resistors.
 *
 ************************************************************************/
static void configure_pull_resistors
    (
    const gpio_type   * gpio,
    pull_resistor_type  cfg
    )
{
    /*--------------------------------------------------------
    Local variables
    --------------------------------------------------------*/
    uint32_t        pin;
    GPIO_TypeDef  * port;

    /*--------------------------------------------------------
    Initialize Variables
    --------------------------------------------------------*/
    pin  = gpio->pin;
    port = gpio->port;

    /*--------------------------------------------------------
    Configure the pull up and pull down resistors.
    --------------------------------------------------------*/
    port->PUPDR &= ~(3 << (pin * 2));
    switch( cfg )
    {
        case PULL_RESISTOR_UP:
            port->PUPDR |= 1 << (pin * 2);
            break;

        case PULL_RESISTOR_DOWN:
            port->PUPDR |= 2 << (pin * 2);
            break;

        case PULL_RESISTOR_NONE:
        default:
            /*------------------------------------------------
            Do nothing, bits already cleared.
            ------------------------------------------------*/
            break;
    }

} /* configure_pull_resistors */


/*************************************************************************
 *
 *  Procedure:
 *      enable_port
 *
 *  Description:
 *      Enable clock signal to the given port.
 *
 ************************************************************************/
static void enable_port
    (
    const gpio_type   * gpio
    )
{
    /*--------------------------------------------------------
    Initialize Variables
    --------------------------------------------------------*/
    switch( (uint32_t)gpio->port )
    {
        case (uint32_t)GPIOA:
            RCC->AHBENR |=  RCC_AHBENR_GPIOAEN;
            break;
        case (uint32_t)GPIOB:
            RCC->AHBENR |=  RCC_AHBENR_GPIOBEN;
            break;
        case (uint32_t)GPIOC:
            RCC->AHBENR |=  RCC_AHBENR_GPIOCEN;
            break;
        case (uint32_t)GPIOD:
            RCC->AHBENR |=  RCC_AHBENR_GPIODEN;
            break;
    }

} /* enable_port */

