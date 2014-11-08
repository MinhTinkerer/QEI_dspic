
/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#include <p30Fxxxx.h>      /* Device header file */

/******************************************************************************/
/* Configuration Bits                                                         */
/*                                                                            */
/* This is not all available configuration bits for all dsPIC devices.        */
/* Refer to the dsPIC device specific .h file in the compiler                 */
/* support\dsPIC30F\h directory for complete options specific to the device   */
/* selected.  For additional information about what the hardware              */
/* configurations mean in terms of device operation, refer to the device      */
/* datasheet 'Special Features' chapter.                                      */
/*                                                                            */
/******************************************************************************/

/* TODO Fill in your configuration bits here and remove the #if 0. */

/* The general style is below: */

//#if 0

_FOSC(XT_PLL8 & CSW_FSCM_OFF); /* Enables internal oscillator and clock sw/mon */
_FBORPOR(PWRT_OFF & BORV_27 & PBOR_OFF & MCLR_EN); /* Ext MCLR, no brownout */
_FWDT(WDT_OFF); /* Turns off watchdog timer */
_FICD(PGD);
//_FICD(ICS_PGD1); /* Selects debug channel channel */

//#endif

