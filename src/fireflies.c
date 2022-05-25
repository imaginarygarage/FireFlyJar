/*********************************************************************************
 *
 *  firefly.c
 *
 *  Created on: Feb 22, 2020
 *      Author:
 *       Brief:
 *
 ********************************************************************************/


/*--------------------------------------------------------------------------------
                                   INCLUDES
--------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "system.h"
#include "leds.h"


/*--------------------------------------------------------------------------------
                                LITERAL CONSTANTS
--------------------------------------------------------------------------------*/

#define NUMBER_OF_FIREFLIES     ( LED_COUNT )
#define FIREFLY_TIMESTEP        ( LED_COUNT )
#define FIREFLY_DELAY_MAX       ( 12000 )
#define FIREFLY_DELAY_MIN       ( 1000 )
#define FIREFLY_SMOOTHING_MAX   ( 500 )
#define FIREFLY_SMOOTHING_MIN   ( 50  )
#define FIREFLY_FLASHPOINTS_MAX ( 15  )


/*--------------------------------------------------------------------------------
                                     MACROS
--------------------------------------------------------------------------------*/

#define rand_range( min, max )  ( rand() % ( ( max ) - ( min ) + 1 ) + ( min ) )


/*--------------------------------------------------------------------------------
                                       TYPES
--------------------------------------------------------------------------------*/

/*------------------------------------------------------------
Flash IDs, used to index into the correct flash pattern
------------------------------------------------------------*/
typedef uint8_t flash_id_type;
enum
{
    FLASH_PHOTINUS_PALLENS,
    FLASH_PHOTINUS_LEWISI,
    FLASH_PHOTINUS_AMPLUS,
    FLASH_PHOTINUS_XANTHOPHOTIS,
    FLASH_PHOTURIS_JAMAICENSIS,
    FLASH_PHOTINUS_LEUCOPYGE,

    FLASH_COUNT,
    FLASH_FIRST = FLASH_PHOTINUS_PALLENS,
    FLASH_LAST = FLASH_PHOTINUS_LEUCOPYGE,
};

/*------------------------------------------------------------
Flash brightness
------------------------------------------------------------*/
typedef int32_t flash_brightness_type;

/*------------------------------------------------------------
Flash points, the building blocks of a flash pattern.
------------------------------------------------------------*/
typedef struct
{
    flash_brightness_type
                    target;     /* Target brightness        */
    int32_t         time;       /* Transition time          */
}flash_point_type;

/*------------------------------------------------------------
Firefly state data
------------------------------------------------------------*/
typedef struct
{
    flash_brightness_type
                    brightness; /* Firefly brightness       */
    boolean         flashing;   /* Firefly is flashing      */
    flash_id_type   flash_id;   /* Type of flash pattern    */
    int32_t         flash_time; /* Time into flash pattern  */
    int32_t         smoothing;  /* Flash pattern smoothing  */
    int32_t         delay_len;  /* Delay length             */
    int32_t         delay_time; /* Time into delay          */
}firefly_type;


/*--------------------------------------------------------------------------------
                                    MEMORY CONSTANTS
--------------------------------------------------------------------------------*/

/*------------------------------------------------------------
Flash pattern definitions. Each flash is assumed to start at 0
brightness. Each flash point in the pattern then defines the
next target brightness and the amount of time allowed to reach
that brightness. The flash is over when the target brightness
reaches zero.

Flash patterns based on research by McDermott and Buck (1959)
------------------------------------------------------------*/
static const flash_point_type flash_patterns[FLASH_COUNT][FIREFLY_FLASHPOINTS_MAX] =
{
    /*--------------------------------------------------------
    Flash of the Photinus pallens
    --------------------------------------------------------*/
    {/* target, time */
        {  800,  300 },
        { 1000,  100 },
        {  800,  200 },
        {    0,  400 }
    },

    /*--------------------------------------------------------
    Flash of the Photinus lewisi
    --------------------------------------------------------*/
    {/* target, time */
        {  500,  100 },
        {  500,  800 },
        {    0,  100 }
    },

    /*--------------------------------------------------------
    Flash of the Photinus amplus
    --------------------------------------------------------*/
    {/* target, time */
        { 1000,  100 },
        {    1,  100 },
        {    1,  100 },
        { 1000,  100 },
        {    0,  100 }
    },

    /*--------------------------------------------------------
    Flash of the Photinus xanthophotis
    --------------------------------------------------------*/
    {/* target, time */
        { 1000,  200 },
        {    1,  100 },
        {    1,  200 },
        {  300,  100 },
        {    1,  100 },
        {  300,  100 },
        {    0,  100 }
    },

    /*--------------------------------------------------------
    Flash of the Photuris jamaicensis
    --------------------------------------------------------*/
    {/* target, time */
        {  500,   50 },
        {  500,  200 },
        {    0,   50 }
    },

    /*--------------------------------------------------------
    Flash of the Photinus leucopyge
    --------------------------------------------------------*/
    {/* target, time */
        { 1000,   50 },
        { 1000,  200 },
        {    0,   50 }
    },
};


/*--------------------------------------------------------------------------------
                                 GLOBAL VARIABLES
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
                                 STATIC VARIABLES
--------------------------------------------------------------------------------*/

volatile static firefly_type s_fireflies[ NUMBER_OF_FIREFLIES ];


/*--------------------------------------------------------------------------------
                                    PROCEDURES
--------------------------------------------------------------------------------*/

static int32_t calculate_flash_length
    (
    flash_id_type   flash_type
    );

static flash_brightness_type calculate_brightness_smoothed
    (
    flash_id_type   flash_id,
    int32_t         flash_time,
    int16_t         smoothing
    );

static flash_brightness_type calculate_brightness_unsmoothed
    (
    flash_id_type   flash_type,
    int32_t         flash_time
    );

static void firefly_periodic_callback
    (
    void
    );

static void firefly_step
    (
    firefly_type * const    firefly,
    uint16_t                time_step
    );


/*************************************************************************
 *
 *  Procedure:
 *      firefly_init
 *
 *  Description:
 *      Initialize firefly data, register periodic callback.
 *
 ************************************************************************/
void firefly_init
    (
    void
    )
{
    /*--------------------------------------------------------
    Local Variables
    --------------------------------------------------------*/
    uint32_t        i;
    firefly_type  * firefly;

    /*--------------------------------------------------------
    TODO: seed random number generator with unique value?
    --------------------------------------------------------*/
    //srand( NUM );

    /*--------------------------------------------------------
    Initialize all fireflies with random delays
    --------------------------------------------------------*/
    for( i = 0; i < count_of_array( s_fireflies ); i++ )
    {
        firefly = (firefly_type *)&s_fireflies[ i ];
        firefly->brightness = 0;
        firefly->delay_len = rand_range( FIREFLY_DELAY_MIN, FIREFLY_DELAY_MAX );
        firefly->delay_time = 0;
        firefly->flash_id = FLASH_FIRST;
        firefly->flash_time = 0;
        firefly->flashing = FALSE;
        firefly->smoothing = FIREFLY_SMOOTHING_MIN;
    }

    /*--------------------------------------------------------
    Register periodic callback function
    --------------------------------------------------------*/
    system_add_task( firefly_periodic_callback, FIREFLY_TIMESTEP );

}   /* firefly_init() */


/*************************************************************************
 *
 *  Procedure:
 *      calculate_flash_length
 *
 *  Description:
 *      Calculate the full run time of a given flash pattern.
 *
 ************************************************************************/
static int32_t calculate_flash_length
    (
    flash_id_type   flash_type
    )
{
    /*--------------------------------------------------------
    Local Variables
    --------------------------------------------------------*/
    int32_t                     flash_length;
    const flash_point_type    * flash_pattern;
    uint32_t                    i;

    /*--------------------------------------------------------
    Get start of flash pattern
    --------------------------------------------------------*/
    flash_pattern = flash_patterns[ flash_type ];

    /*--------------------------------------------------------
    Iterate over all flash points to get cumulative run time.
    --------------------------------------------------------*/
    flash_length = 0;
    for( i = 0; i == 0 || flash_pattern[ i - 1 ].target != 0; i++ )
    {
        flash_length += flash_pattern[ i ].time;
    }

    return( flash_length );

} /* calculate_flash_length() */


/*************************************************************************
 *
 *  Procedure:
 *      calculate_brightness_smoothed
 *
 *  Description:
 *      Calculate the smoothed brightness of a given flash type at a
 *      specified flash time.
 *
 ************************************************************************/
static flash_brightness_type calculate_brightness_smoothed
    (
    flash_id_type   flash_id,
    int32_t         flash_time,
    int16_t         smoothing
    )
{
    /*--------------------------------------------------------
    Local Variables
    --------------------------------------------------------*/
    flash_brightness_type   brightness;
    int32_t                 flash_length;
    const flash_point_type* flash_pattern;
    const flash_point_type* flash_point;
    int32_t                 flash_point_start_time;
    int32_t                 flash_point_end_time;
    int32_t                 half_smooth;
    int32_t                 smooth_start;
    int32_t                 smooth_end;
    int32_t                 i;
    int32_t                 b1;
    int32_t                 b2;
    int32_t                 t1;
    int32_t                 t2;

    /*--------------------------------------------------------
    Calculate smoothing period.
    --------------------------------------------------------*/
    half_smooth = smoothing >> 1;
    smoothing = ( half_smooth << 1 ) + 1;
    smooth_start = flash_time - half_smooth;
    smooth_end = smooth_start + smoothing;

    /*--------------------------------------------------------
    Calculate flash length.
    --------------------------------------------------------*/
    flash_length = calculate_flash_length( flash_id );

    /*--------------------------------------------------------
    Confirm smoothing windows contains flash activity.
    --------------------------------------------------------*/
    if( smooth_end < 0
     || smooth_start > flash_length )
    {
        /*----------------------------------------------------
        Smoothing window does not overlap active flash range.
        ----------------------------------------------------*/
        return( 0 );
    }

    /*--------------------------------------------------------
    Calculate average brightness over smoothing window.
    --------------------------------------------------------*/
    brightness = 0;
    flash_point_start_time = 0;
    flash_pattern = flash_patterns[flash_id];
    for( i = 0; i == 0 || flash_pattern[ i - 1 ].target != 0; i++ )
    {
        /*----------------------------------------------------
        Get current flash point data.
        ----------------------------------------------------*/
        flash_point = &flash_pattern[ i ];
        flash_point_end_time = flash_point_start_time + flash_point->time;

        /*----------------------------------------------------
        Get interval start time.
        ----------------------------------------------------*/
        if( smooth_start >= flash_point_start_time
         && smooth_start < flash_point_end_time )
        {
            t1 = smooth_start;
        }
        else if( smooth_start < flash_point_start_time )
        {
            t1 = flash_point_start_time;
        }
        else
        {
            flash_point_start_time = flash_point_end_time;
            continue;
        }

        /*----------------------------------------------------
        Add weighted brightness for interval.
        ----------------------------------------------------*/
        b1 = calculate_brightness_unsmoothed( flash_id, t1 );
        t2 = min_val( flash_point_end_time, smooth_end );
        b2 = calculate_brightness_unsmoothed( flash_id, t2 );
        brightness += (b2 + b1) * (t2 - t1);

        /*----------------------------------------------------
        Increment start time if smoothing period is not over.
        ----------------------------------------------------*/
        if( t2 == smooth_end )
        {
            break;
        }
        flash_point_start_time = t2;

    }
    brightness /= 2 * smoothing;

    return( brightness );

} /* calculate_brightness_smoothed() */


/*************************************************************************
 *
 *  Procedure:
 *      calculate_brightness_unsmoothed
 *
 *  Description:
 *      Calculate the unsmoothed brightness of a given flash type at a
 *      given flash time.
 *
 ************************************************************************/
static flash_brightness_type calculate_brightness_unsmoothed
    (
    flash_id_type   flash_type,
    int32_t         flash_time
    )
{
    /*--------------------------------------------------------
    Local Variables
    --------------------------------------------------------*/
    flash_brightness_type       brightness;
    const flash_point_type    * flash_pattern;
    flash_point_type            flash_point_prv;
    flash_point_type            flash_point_cur;
    int32_t                     flash_point_end_time;
    int32_t                     i;

    /*--------------------------------------------------------
    Validate input
    --------------------------------------------------------*/
    if( flash_time < 0 )
    {
        return( 0 );
    }

    /*--------------------------------------------------------
    Initialize Variables
    --------------------------------------------------------*/
    clear_struct( flash_point_prv );
    clear_struct( flash_point_cur );

    /*--------------------------------------------------------
    Get start of flash pattern
    --------------------------------------------------------*/
    flash_pattern = flash_patterns[ flash_type ];

    /*--------------------------------------------------------
    Get current and previous flash points
    --------------------------------------------------------*/
    flash_point_end_time = 0;
    for( i = 0; i == 0 || flash_pattern[ i - 1 ].target != 0; i++ )
    {
        flash_point_end_time += flash_pattern[ i ].time;
        if( flash_time < flash_point_end_time )
        {
            flash_point_cur = flash_pattern[ i ];
            if( i != 0 )
            {
                flash_point_prv = flash_pattern[ i - 1 ];
            }
            break;
        }
    }

    /*--------------------------------------------------------
    Calculate brightness at flash_time
    --------------------------------------------------------*/
    if( flash_point_cur.time )
    {
        brightness = ( flash_point_cur.time - ( flash_point_end_time - flash_time ) ) * (flash_point_cur.target - flash_point_prv.target) / flash_point_cur.time + flash_point_prv.target;
    }
    else
    {
        brightness = 0;
    }

    return( brightness );

} /* calculate_brightness_unsmoothed() */


/*************************************************************************
 *
 *  Procedure:
 *      firefly_periodic_callback
 *
 *  Description:
 *      Periodic callback to update all fireflies.
 *
 ************************************************************************/
static void firefly_periodic_callback
    (
    void
    )
{
    /*--------------------------------------------------------
    Local Variables
    --------------------------------------------------------*/
    firefly_type * firefly;
    uint32_t                i;

    /*--------------------------------------------------------
    Update all fireflies
    --------------------------------------------------------*/
    for( i = 0; i < count_of_array( s_fireflies ); i++ )
    {
        firefly = (firefly_type *)&s_fireflies[ i ];

        if( firefly->flashing )
        {
            /*------------------------------------------------
            Step through active flash
            ------------------------------------------------*/
            firefly_step( firefly, FIREFLY_TIMESTEP );

            /*------------------------------------------------
            Update corresponding LEDs
            ------------------------------------------------*/
            led_set_brightness( i, firefly->brightness * 150 / 1000 );
        }
        else if( firefly->delay_len <= firefly->delay_time )
        {
            /*------------------------------------------------
            Initialize new flash
            ------------------------------------------------*/
            firefly->flash_id = rand_range( FLASH_FIRST, FLASH_LAST );
            firefly->smoothing = rand_range( FIREFLY_SMOOTHING_MIN, FIREFLY_SMOOTHING_MAX );
            firefly->flash_time = -( firefly->smoothing / 2 );
            firefly->flashing = TRUE;

            /*------------------------------------------------
            Update post flash delay
            ------------------------------------------------*/
            firefly->delay_len = rand_range( FIREFLY_DELAY_MIN, FIREFLY_DELAY_MAX );
            firefly->delay_time = 0;
        }
        else
        {
            /*------------------------------------------------
            Update delay counter
            ------------------------------------------------*/
            firefly->delay_time += FIREFLY_TIMESTEP;
        }
    }

}   /* firefly_periodic_callback() */


/*************************************************************************
 *
 *  Procedure:
 *      firefly_step
 *
 *  Description:
 *      Advance a firefly's state by a specified time step.
 *
 ************************************************************************/
static void firefly_step
    (
    firefly_type * const    firefly,
    uint16_t                time_step
    )
{
    /*--------------------------------------------------------
    Update flash time
    --------------------------------------------------------*/
    firefly->flash_time += time_step;

    /*--------------------------------------------------------
    Calculate new smoothed brightness
    --------------------------------------------------------*/
    firefly->brightness = calculate_brightness_smoothed( firefly->flash_id, firefly->flash_time, firefly->smoothing );

    /*--------------------------------------------------------
    Update flashing flag if flash has completed
    --------------------------------------------------------*/
    if( firefly->brightness == 0
     && firefly->smoothing < firefly->flash_time )
    {
        firefly->flashing = FALSE;
    }

}   /* firefly_step() */

