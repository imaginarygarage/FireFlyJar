/*********************************************************************************
 *
 *  system.h
 *
 *  Created on: Aug 11, 2019
 *      Author: Tanner Leland
 *       Brief: System initialization and coordination.
 *
 ********************************************************************************/

#ifndef SYSTEM_H_
#define SYSTEM_H_

/*--------------------------------------------------------------------------------
                                   INCLUDES
--------------------------------------------------------------------------------*/

#include <string.h>


/*--------------------------------------------------------------------------------
                                LITERAL CONSTANTS
--------------------------------------------------------------------------------*/

#define SYSTEM_TASKS_MAX    ( 10 )      /* Maximum number of system tasks       */
#define SYSTICK_HZ          ( 1000 )    /* Number of systick events per second  */


/*--------------------------------------------------------------------------------
                                     MACROS
--------------------------------------------------------------------------------*/

#define clear_struct( s )           ( memset( (void *)&(s), 0, sizeof( s ) ) )
#define clear_array( a )            ( memset( (void *)(a), 0, sizeof( a ) ) )
#define count_of_array( a )         ( sizeof( a ) / sizeof( *(a) ) )
#define min_val( val1, val2 )       ( (val1) < (val2) ? (val1) : (val2) )
#define max_val( val1, val2 )       ( (val1) > (val2) ? (val1) : (val2) )
#define limit_val( val, min, max)   ( min_val( max_val( val, min ), max ) )


/*--------------------------------------------------------------------------------
                                      TYPES
--------------------------------------------------------------------------------*/

/*------------------------------------------------------------
Task Function Pointer
------------------------------------------------------------*/
typedef void (*task_ptr_type)( void );

/*------------------------------------------------------------
Boolean type
------------------------------------------------------------*/
typedef char boolean;
enum
{
    FALSE,
    TRUE,
};


/*--------------------------------------------------------------------------------
                                GLOBAL VARIABLES
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
                                   PROCEDURES
--------------------------------------------------------------------------------*/

void system_add_task
    (
    task_ptr_type   tsk,    /* Pointer to periodic task function        */
    uint32_t        prd     /* Task period in milliseconds              */
    );

void system_init
    (
    void
    );

void system_remove_task
    (
    task_ptr_type   tsk     /* Pointer to periodic task function        */
    );


#endif /* SYSTEM_H_ */
