/*********************************************************************************
 *
 *  system.c
 *
 *  Created on: Aug 11, 2019
 *      Author: Tanner Leland
 *       Brief:
 *
 ********************************************************************************/


/*--------------------------------------------------------------------------------
                                   INCLUDES
--------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stm32f3xx.h>

#include "system.h"

/*--------------------------------------------------------------------------------
                                      TYPES
--------------------------------------------------------------------------------*/

/*------------------------------------------------------------
Task List Node
------------------------------------------------------------*/
typedef struct
{
    task_ptr_type   task;
    uint32_t        period;
}task_list_type;

/*--------------------------------------------------------------------------------
                               GLOBAL VARIABLES
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
                               STATIC VARIABLES
--------------------------------------------------------------------------------*/

/*------------------------------------------------------------
System Tasks
------------------------------------------------------------*/
volatile static task_list_type s_task_list[ SYSTEM_TASKS_MAX ];

/*--------------------------------------------------------------------------------
                                  PROCEDURES
--------------------------------------------------------------------------------*/

static void execute_tasks
    (
    void
    );


/*************************************************************************
 *
 *  Procedure:
 *      system_add_task
 *
 *  Description:
 *      Register a new periodic system task.
 *
 ************************************************************************/
void system_add_task
    (
    task_ptr_type   tsk,    /* Pointer to periodic task function        */
    uint32_t        prd     /* Task period in milliseconds              */
    )
{
    /*--------------------------------------------------------
    Local variables
    --------------------------------------------------------*/
    uint32_t        i;      /* loop counter                 */

    /*--------------------------------------------------------
    Register new task
    --------------------------------------------------------*/
    for( i = 0; i < count_of_array( s_task_list ); i++ )
    {
        /*----------------------------------------------------
        Assign to first empty slot
        ----------------------------------------------------*/
        if( s_task_list[ i ].task == NULL )
        {
            s_task_list[ i ].task = tsk;
            s_task_list[ i ].period = max_val( prd, 1 );
            break;
        }

        /*----------------------------------------------------
        Skip registration if task already active
        ----------------------------------------------------*/
        if( s_task_list[ i ].task == tsk )
        {
            break;
        }
    }

}   /* system_add_task() */


/*************************************************************************
 *
 *  Procedure:
 *      system_init
 *
 *  Description:
 *      Initializes the system structure and configures the system clock.
 *
 *  References:
 *
 *
 ************************************************************************/
void system_init
    (
    void
    )
{
    /*--------------------------------------------------------
    Configure flash latency to allow sufficient flash access
    time. Flash latency must be at least 2 wait states for
    clock speeds between 48MHz and 72 MHz.
    --------------------------------------------------------*/
    FLASH->ACR |= FLASH_ACR_LATENCY_1;

    /*--------------------------------------------------------
    Set PLL to operate at 16x and enable PLL. Default input to
    PLL is HSI clock divided by 2. HSI clock is an 8MHz
    internal RC oscillator. Resulting operating frequency
    should therefore be 64MHz.
    --------------------------------------------------------*/
    RCC->CFGR |= RCC_CFGR_PLLMUL;
    RCC->CR   |= RCC_CR_PLLON;

    /*--------------------------------------------------------
    Wait for hardware to indicate PLL is ready
    --------------------------------------------------------*/
    while( !(RCC->CR & RCC_CR_PLLRDY) );

    /*--------------------------------------------------------
    Set system clock source to PLL
    --------------------------------------------------------*/
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    /*--------------------------------------------------------
    Wait for hardware to indicate that the system clock source
    is PLL.
    --------------------------------------------------------*/
    while( !(RCC->CFGR & RCC_CFGR_SWS_PLL) );

    /*--------------------------------------------------------
    Update SystemCoreClock to reflect current system clock
    --------------------------------------------------------*/
    SystemCoreClockUpdate();

    /*--------------------------------------------------------
    Configure SysTick to operate at SYSTICK_HZ
    --------------------------------------------------------*/
    SysTick_Config( SystemCoreClock / SYSTICK_HZ );

}   /* system_init() */


/*************************************************************************
 *
 *  Procedure:
 *      system_remove_task
 *
 *  Description:
 *      Remove a periodic system task.
 *
 ************************************************************************/
void system_remove_task
    (
    task_ptr_type   tsk     /* Pointer to periodic task function        */
    )
{
    /*--------------------------------------------------------
    Local variables
    --------------------------------------------------------*/
    uint32_t    i;      /* loop counter                     */

    /*--------------------------------------------------------
    Remove task
    --------------------------------------------------------*/
    for( i = 0; i < count_of_array( s_task_list ); i++ )
    {
        /*----------------------------------------------------
        Remove any instances of task
        ----------------------------------------------------*/
        if( s_task_list[ i ].task == tsk )
        {
            s_task_list[ i ].task = NULL;
            s_task_list[ i ].period = 0;
        }
    }

}   /* system_remove_task() */


/*************************************************************************
 *
 *  Procedure:
 *      SysTick_Handler
 *
 *  Description:
 *      SysTick interrupt routine.
 *
 ************************************************************************/
void SysTick_Handler
    (
    void
    )
{
    execute_tasks();

}   /* SysTick_Handler() */


/*************************************************************************
 *
 *  Procedure:
 *      execute_tasks
 *
 *  Description:
 *      Execute periodic system task. This function runs on every SysTick.
 *      It compares the period of each task to a running counter in order
 *      to determine which tasks to execute.
 *
 ************************************************************************/
static void execute_tasks
    (
    void
    )
{
    /*--------------------------------------------------------
    Local static variables
    --------------------------------------------------------*/
    static uint32_t counter = 0;    /* persistent counter   */

    /*--------------------------------------------------------
    Local variables
    --------------------------------------------------------*/
    int                 i;          /* task index           */
    volatile task_list_type
                      * cur_task;   /* pointer to task      */

    /*--------------------------------------------------------
    Increment counter
    --------------------------------------------------------*/
    counter++;

    /*--------------------------------------------------------
    Execute tasks
    --------------------------------------------------------*/
    for( i = 0; i < count_of_array( s_task_list ); i++ )
    {
        cur_task = &s_task_list[ i ];

        /*----------------------------------------------------
        Execute only if current task's period has elapsed.
        ----------------------------------------------------*/
        if( ((counter + i) % cur_task->period) == 0
         && cur_task->task != NULL )
        {
            cur_task->task();
        }
    }

}   /* execute_tasks() */

