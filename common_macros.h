 /******************************************************************************
 *
 * Module: Common - Macros
 *
 * File Name: common_macros.h
 *
 * Description: Commonly used Macros
 *
 * Author: Omar Tarek
 *
 *******************************************************************************/

#ifndef COMMON_MACROS
#define COMMON_MACROS

/*******************************************************************************
 *                                 MACROS	                                   *
 *******************************************************************************/

/* Set a certain bit in any register */
#define SET_BIT(REG,BIT) (REG|=(1<<BIT))

/* Clear a certain bit in any register */
#define CLEAR_BIT(REG,BIT) (REG&=(~(1<<BIT)))

/* Toggle a certain bit in any register */
#define TOGGLE_BIT(REG,BIT) (REG^=(1<<BIT))

/* Check if a specific bit is set in any register and return true if yes */
#define BIT_IS_SET(REG,BIT) ( REG & (1<<BIT) )

/* Check if a specific bit is cleared in any register and return true if yes */
#define BIT_IS_CLEAR(REG,BIT) ( !(REG & (1<<BIT)) )

/* Gets a certain bit whether 0 or 1 */
#define GET_BIT(REG, BIT) ((REG & (1 << BIT)) >> BIT)

#endif
