/*********************************************************************************
 *
 *  main.c
 *
 *  Created on: Aug 11, 2019
 *      Author: Tanner Leland
 *       Brief: main entry point
 *
 ********************************************************************************/


/*--------------------------------------------------------------------------------
                                   INCLUDES
--------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stm32f3xx.h>

#include "system.h"
#include "leds.h"
#include "fireflies.h"
#include "gpio.h"
#include "touch.h"


/*--------------------------------------------------------------------------------
                                LITERAL CONSTANTS
--------------------------------------------------------------------------------*/

#define TIMEOUT_MINUTES     ( 15 )
#define TIMEOUT_SECONDS     ( TIMEOUT_MINUTES * 60 )
#define TIMEOUT_MS          ( TIMEOUT_SECONDS * 1000 )


/*--------------------------------------------------------------------------------
                                      TYPES
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
                               MEMORY_CONSTANTS
--------------------------------------------------------------------------------*/

static const gpio_type hold_power_io = { GPIOA,  1 };


/*--------------------------------------------------------------------------------
                               GLOBAL VARIABLES
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
                               STATIC VARIABLES
--------------------------------------------------------------------------------*/

volatile static uint32_t s_timeout_counter;


/*--------------------------------------------------------------------------------
                                  PROCEDURES
--------------------------------------------------------------------------------*/

static void main_timeout_callback
    (
    void
    );


/*************************************************************************
 *
 *  Procedure:
 *      main
 *
 *  Description:
 *      Main entry point for this project.
 *
 ************************************************************************/
int main
    (
    void
    )
{
    /*--------------------------------------------------------
    Set 'Hold power' pin high before touch controller goes
    low to keep the lights on until we decide to turn off.
    --------------------------------------------------------*/
    gpio_cfg_output( &hold_power_io );
    gpio_output_set( &hold_power_io, GPIO_STATE_HIGH );

    /*--------------------------------------------------------
    Initialize system
    --------------------------------------------------------*/
    system_init();
    led_init();
    firefly_init();
    touch_init();

    /*--------------------------------------------------------
    Start timeout counter
    --------------------------------------------------------*/
    s_timeout_counter = 0;
    system_add_task( main_timeout_callback, 1 );

    while( 1 )
    {
        /*----------------------------------------------------
        Do nothing until interrupted.
        ----------------------------------------------------*/
    }

    return( 0 );

} /* main() */


/*************************************************************************
 *
 *  Procedure:
 *      main_timeout_callback
 *
 *  Description:
 *      A callback function to end firefly activity if the jar has not
 *      been touched in TIMEOUT_MS.
 *
 ************************************************************************/
static void main_timeout_callback
    (
    void
    )
{
    /*--------------------------------------------------------
    Reset the timeout counter if a touch event was sensed,
    otherwise increment the counter.
    --------------------------------------------------------*/
    if( touch_read() == TOUCH_STATE_ACTIVE )
    {
        s_timeout_counter = 0;
    }
    else
    {
        s_timeout_counter++;
    }

    /*--------------------------------------------------------
    Check if it's time to shut down.
    --------------------------------------------------------*/
    if( s_timeout_counter >= TIMEOUT_MS )
    {
        gpio_output_set( &hold_power_io, GPIO_STATE_LOW );
    }

} /* main_timeout_callback() */
