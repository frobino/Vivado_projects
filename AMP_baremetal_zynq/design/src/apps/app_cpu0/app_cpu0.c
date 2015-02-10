/*
 * Copyright (c) 2009 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/*
 * helloworld.c: simple test application
 */

/***************************** Include Files ********************************/
#include <stdio.h>
#include "xil_io.h"
#include "xil_mmu.h"
#include "xil_exception.h"
#include "xpseudo_asm.h"
#include "xscugic.h"


/************************** Constant Definitions ****************************/
#define INTC		    XScuGic
#define INTC_DEVICE_ID	XPAR_PS7_SCUGIC_0_DEVICE_ID
#define INTC_HANDLER	XScuGic_InterruptHandler

#define COMM_VAL  (*(volatile unsigned long *)(0xFFFF0000))

#define APP_CPU1_ADDR	0x02000000

/**************************** Type Definitions ******************************/

/***************** Macros (Inline Functions) Definitions ********************/

/************************** Variable Definitions ****************************/

/*
 * Assign the driver structures for the interrupt controller
 */
INTC   IntcInstancePtr;

/************************** Function Prototypes *****************************/
static int  SetupIntrSystem(INTC *IntcInstancePtr);


/*****************************************************************************/
/**
*
* main()
*	- Runs example.
*
*
* @param	tbd
*
* @return
*		- tbd
*
* @note		tbd
*
******************************************************************************/
int main()
{
	int Status;

    //Disable cache on OCM
    Xil_SetTlbAttributes(0xFFFF0000,0x14de2);           // S=b1 TEX=b100 AP=b11, Domain=b1111, C=b0, B=b0

    //Disable cache on fsbl vector table location
    Xil_SetTlbAttributes(0x00000000,0x14de2);           // S=b1 TEX=b100 AP=b11, Domain=b1111, C=b0, B=b0


    COMM_VAL = 0;

	// Initialize the SCU Interrupt Distributer (ICD)
	Status = SetupIntrSystem(&IntcInstancePtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


    print("CPU0: writing startaddress for cpu1\n\r");

    {
    	/*
    	 *  Reset and start CPU1
    	 *  - Application for cpu1 exists at 0x00000000 per cpu1 linkerscript
    	 *
    	 */
		#include "xil_misc_psreset_api.h"
		#include "xil_io.h"

    	#define A9_CPU_RST_CTRL		(XSLCR_BASEADDR + 0x244)
		#define A9_RST1_MASK 		0x00000002
		#define A9_CLKSTOP1_MASK	0x00000020
		#define CPU1_CATCH			0x00000024

		#define XSLCR_LOCK_ADDR		(XSLCR_BASEADDR + 0x4)
		#define XSLCR_LOCK_CODE		0x0000767B

    	u32 RegVal;


    	/*
    	 * Setup cpu1 catch address with starting address of app_cpu1. The FSBL initialized the vector table at 0x00000000
    	 * using a boot.S that checks for cpu number and jumps to the address stored at the
    	 * end of the vector table in cpu0_catch and cpu1_catch entries.
    	 * Note: Cache has been disabled at the beginning of main(). Otherwise
		 * a cache flush would have to be issued after this write
    	 */
    	Xil_Out32(CPU1_CATCH, APP_CPU1_ADDR);


    	/* Unlock the slcr register access lock */
    	Xil_Out32(XSLCR_UNLOCK_ADDR, XSLCR_UNLOCK_CODE);

    	//    the user must stop the associated clock, de-assert the reset, and then restart the clock. During a
    	//    system or POR reset, hardware automatically takes care of this. Therefore, a CPU cannot run the code
    	//    that applies the software reset to itself. This reset needs to be applied by the other CPU or through
    	//    JTAG or PL. Assuming the user wants to reset CPU1, the user must to set the following fields in the
    	//    slcr.A9_CPU_RST_CTRL (address 0xF8000244) register in the order listed:
    	//    1. A9_RST1 = 1 to assert reset to CPU0
    	//    2. A9_CLKSTOP1 = 1 to stop clock to CPU0
    	//    3. A9_RST1 = 0 to release reset to CPU0
    	//    4. A9_CLKSTOP1 = 0 to restart clock to CPU0

    	/* Assert and deassert cpu1 reset and clkstop using above sequence*/
    	RegVal = 	Xil_In32(A9_CPU_RST_CTRL);
    	RegVal |= A9_RST1_MASK;
    	Xil_Out32(A9_CPU_RST_CTRL, RegVal);
    	RegVal |= A9_CLKSTOP1_MASK;
    	Xil_Out32(A9_CPU_RST_CTRL, RegVal);
    	RegVal &= ~A9_RST1_MASK;
		Xil_Out32(A9_CPU_RST_CTRL, RegVal);
    	RegVal &= ~A9_CLKSTOP1_MASK;
		Xil_Out32(A9_CPU_RST_CTRL, RegVal);

    	/* lock the slcr register access */
    	Xil_Out32(XSLCR_LOCK_ADDR, XSLCR_LOCK_CODE);
    }



    while(1){

		print("CPU0: Hello World CPU 0\n\r");
		COMM_VAL = 1;
		while(COMM_VAL == 1);

    }

    return 0;
}


/*****************************************************************************/
/**
*
* This function setups initializes the interrupt system.
*
* @param	IntcInstancePtr is a pointer to the instance of the Intc driver.
* @param	PeriphInstancePtr is a pointer to the instance of peripheral driver.
* @param	IntrId is the Interrupt Id of the peripheral interrupt
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note		None.
*
******************************************************************************/
static int SetupIntrSystem(INTC *IntcInstancePtr)
{
	int Status;


	XScuGic_Config *IntcConfig;

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
					IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Initialize the  exception table
	 */
	Xil_ExceptionInit();

	/*
	 * Register the interrupt controller handler with the exception table
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			 (Xil_ExceptionHandler)INTC_HANDLER,
			 IntcInstancePtr);

	/*
	 * Enable non-critical exceptions
	 */
	Xil_ExceptionEnable();


	return XST_SUCCESS;
}
