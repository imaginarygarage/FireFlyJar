/*********************************************************************************
 *
 *  system.h
 *
 *  Created on: Aug 11, 2019
 *      Author: Tanner Leland
 *       Brief: touch
 *
 ********************************************************************************/

#ifndef TOUCH_H_
#define TOUCH_H_

/*--------------------------------------------------------------------------------
                                   INCLUDES
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
                                LITERAL CONSTANTS
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
                                     MACROS
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
                                      TYPES
--------------------------------------------------------------------------------*/

/*------------------------------------------------------------
Touch state
------------------------------------------------------------*/
typedef enum
{
    TOUCH_STATE_INACTIVE,
    TOUCH_STATE_ACTIVE
} touch_state_type;


/*--------------------------------------------------------------------------------
                                GLOBAL VARIABLES
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
                                   PROCEDURES
--------------------------------------------------------------------------------*/

void touch_init
    (
    void
    );

touch_state_type touch_read
    (
    void
    );


#endif /* TOUCH_H_ */
