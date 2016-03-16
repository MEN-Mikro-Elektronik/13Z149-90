/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file   men_z149.h.h
 *
 *        \author david.robinson@men.de
 * 
 *        \brief  DPRAM driver for FPGAs containing a 16z149 unit.
 *
 */
/*-------------------------------[ History ]---------------------------------
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2016 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef _MEN_Z149_DPRAM_
#define _MEN_Z149_DPRAM_

struct dpram_info
{
	uint64_t address;
	uint32_t size;
};

#define DPRAM_INT_FROM_NIOS_NONE            0
#define DPRAM_INT_FROM_NIOS_PENDING         1

#define DPRAM_INT_MODE_FROM_NIOS_POLLING    0
#define DPRAM_INT_MODE_FROM_NIOS_INTERRUPT  1

#define DPRAM_IOC_MAGIC                     'J'

#define DPRAM_IOC_GET_INFO                  _IOR(DPRAM_IOC_MAGIC, 0, struct dpram_info *)
#define DPRAM_IOC_GET_INT_FROM_NIOS         _IOR(DPRAM_IOC_MAGIC, 1, int )
#define DPRAM_IOC_RESET_INT_FROM_NIOS       _IO(DPRAM_IOC_MAGIC, 2)
#define DPRAM_IOC_SET_INT_TO_NIOS           _IO(DPRAM_IOC_MAGIC, 3)
//#define DPRAM_IOC_GET_INT_MODE_FROM_NIOS    _IOR(DPRAM_IOC_MAGIC, 4, int )
//#define DPRAM_IOC_SET_INT_MODE_FROM_NIOS    _IOW(DPRAM_IOC_MAGIC, 5, int )

#endif
