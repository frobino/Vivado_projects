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
#include "xparameters.h"
#include <stdio.h>
#include "xil_io.h"
#include "xil_mmu.h"
#include "xil_cache.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "sleep.h"
// New
#include "xgpio.h"
#define CUSTOM_EXAMPLE


/************************** Constant Definitions ****************************/
#define INTC		    XScuGic
#define INTC_HANDLER	XScuGic_InterruptHandler
#define INTC_DEVICE_ID	XPAR_PS7_SCUGIC_0_DEVICE_ID
#define PL_IRQ_ID       XPS_IRQ_INT_ID

#define IRQ_PCORE_GEN_BASE  XPAR_IRQ_GEN_0_BASEADDR


#define COMM_VAL    (*(volatile unsigned long *)(0xFFFF0000))

//New
#define BTNS_DEVICE_ID		XPAR_AXI_GPIO_0_DEVICE_ID
#define INTC_GPIO_INTERRUPT_ID XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define BTN_INT 			XGPIO_IR_CH1_MASK


/**************************** Type Definitions ******************************/
/**
 * This typedef contains configuration information for the device driver.
 */
typedef struct {
	u16 DeviceId;		/**< Unique ID of device */
	u32 BaseAddress;	/**< Base address of the device */
} Pl_Config;

//New
XGpio BTNInst;
XScuGic INTCInst;
static int btn_value;


/**
 * The driver instance data. The user is required to allocate a
 * variable of this type.
 * A pointer to a variable of this type is then passed to the driver API
 * functions.
 */
typedef struct {
	Pl_Config Config;   /**< Hardware Configuration */
	u32 IsReady;		/**< Device is initialized and ready */
	u32 IsStarted;		/**< Device is running */
} XPlIrq;

/***************** Macros (Inline Functions) Definitions ********************/

/************************** Variable Definitions ****************************/
extern u32 MMUTable;

/**
 * Assign the driver structures for the interrupt controller and
 * PL Core interrupt source
 */
INTC   IntcInstancePtr;
XPlIrq PlIrqInstancePtr;

// Global for IRQ communication to main()
int irq_count;

/************************** Function Prototypes *****************************/

void        Xil_L1DCacheFlush(void);
static int  SetupIntrSystem(INTC *IntcInstancePtr, XPlIrq *PeriphInstancePtr, u16 IntrId);
static void DisableIntrSystem(INTC *IntcInstancePtr, u16 IntrId);
static void PlIntrHandler(void *CallBackRef);

//New
//----------------------------------------------------
// PROTOTYPE FUNCTIONS
//----------------------------------------------------
static void BTN_Intr_Handler(void *baseaddr_p);
static int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);
static int IntcInitFunction(u16 DeviceId, XGpio *GpioInstancePtr);



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


    print("CPU1: init_platform\n\r");

    irq_count = 0;

    #ifdef ORIGINAL_EXAMPLE
    // Initialize driver instance for PL IRQ
    PlIrqInstancePtr.Config.DeviceId = PL_IRQ_ID;
    PlIrqInstancePtr.Config.BaseAddress = IRQ_PCORE_GEN_BASE;
    PlIrqInstancePtr.IsReady = XIL_COMPONENT_IS_READY;
    PlIrqInstancePtr.IsStarted = 0;

	/*
	 * Connect the PL IRQ to the interrupt subsystem so that interrupts
	 * can occur
	 */
	Status = SetupIntrSystem(&IntcInstancePtr,
					&PlIrqInstancePtr,
					PL_IRQ_ID);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
	#endif

	#ifdef CUSTOM_EXAMPLE
	int status;
	//----------------------------------------------------
	// INITIALIZE THE PERIPHERALS & SET DIRECTIONS OF GPIO
	//----------------------------------------------------
	// Initialise Push Buttons
	status = XGpio_Initialize(&BTNInst, BTNS_DEVICE_ID);
	if(status != XST_SUCCESS) return XST_FAILURE;
	// Set all buttons direction to inputs
	XGpio_SetDataDirection(&BTNInst, 1, 0xFF);

	// Initialize interrupt controller
	status = IntcInitFunction(INTC_DEVICE_ID, &BTNInst);
	if(status != XST_SUCCESS) return XST_FAILURE;

	#endif

     while(1){
     	while(COMM_VAL == 0){};
     	if(irq_count > 0) {
         	print("CPU1: Hello World With Interrupt CPU 1\n\r");
         	irq_count = 0;
         	sleep(2);		//Delay so output can be seen
     	} else {
     	print("CPU1: Hello World CPU 1\n\r");
     	}
     	COMM_VAL = 0;

     }

	/*
	 * Disable and disconnect the interrupt system
	 */
	DisableIntrSystem(&IntcInstancePtr, PL_IRQ_ID);



    return 0;
}

/*****************************************************************************/
/**
*
* This function setups the interrupt system such that PL interrupt can occur
* for the peripheral. This function is application specific since the actual
* system may or may not have an interrupt controller. The peripheral device could be
* directly connected to a processor without an interrupt controller. The
* user should modify this function to fit the application.
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
static int SetupIntrSystem(INTC *IntcInstancePtr,
		XPlIrq *PeriphInstancePtr,
				u16 IntrId)
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
	 * Connect the interrupt handler that will be called when an
	 * interrupt occurs for the device.
	 */
	Status = XScuGic_Connect(IntcInstancePtr, IntrId,
				 (Xil_ExceptionHandler)PlIntrHandler,
				 PeriphInstancePtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	/*
	 * Enable the interrupt for the PL device.
	 */
	XScuGic_Enable(IntcInstancePtr, IntrId);


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

/*****************************************************************************/
/**
*
* This function is the Interrupt handler for the PL Interrupt.
* It is called when the PL creates an interrupt and the interrupt gets serviced.
*
* This function sets the global varialbe irq_count=1 and clears the interrupt source.
*
* @param	CallBackRef is a pointer to the callback function.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void PlIntrHandler(void *CallBackRef)
{

	XPlIrq *InstancePtr = (XPlIrq *)CallBackRef;

	/*
	 * Clear the interrupt source
	 */
	Xil_Out32(InstancePtr->Config.BaseAddress, 0);

	irq_count = 1;

}



/*****************************************************************************/
/**
*
* This function disables the interrupts that occur
*
* @param	IntcInstancePtr is the pointer to the instance of INTC driver.
* @param	IntrId is the Interrupt Id of the peripheral
*		value from xparameters.h.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
static void DisableIntrSystem(INTC *IntcInstancePtr, u16 IntrId)
{

	/*
	 * Disconnect and disable the interrupt
	 */
	/* Disconnect the interrupt */
	XScuGic_Disable(IntcInstancePtr, IntrId);
	XScuGic_Disconnect(IntcInstancePtr, IntrId);
}


// New

//----------------------------------------------------
// INTERRUPT HANDLER FUNCTIONS
// - called by the timer, button interrupt, performs
// - LED flashing
//----------------------------------------------------


void BTN_Intr_Handler(void *InstancePtr)
{
	// Disable GPIO interrupts
	XGpio_InterruptDisable(&BTNInst, BTN_INT);
	// Ignore additional button presses
	if ((XGpio_InterruptGetStatus(&BTNInst) & BTN_INT) !=
			BTN_INT) {
			return;
		}
	btn_value = XGpio_DiscreteRead(&BTNInst, 1);
	// All external buttons (NSWE) notify main of interrupt,
	// if centre button pressed the main is not notified
	if(btn_value != 1) irq_count = 1; // Notify
	else irq_count = 0; //Not notify
    (void)XGpio_InterruptClear(&BTNInst, BTN_INT);
    // Enable GPIO interrupts
    XGpio_InterruptEnable(&BTNInst, BTN_INT);
}

//----------------------------------------------------
// INITIAL SETUP FUNCTIONS
//----------------------------------------------------

int InterruptSystemSetup(XScuGic *XScuGicInstancePtr)
{
	// Enable interrupt
	XGpio_InterruptEnable(&BTNInst, BTN_INT);
	XGpio_InterruptGlobalEnable(&BTNInst);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			 	 	 	 	 	 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
			 	 	 	 	 	 XScuGicInstancePtr);
	Xil_ExceptionEnable();


	return XST_SUCCESS;

}

int IntcInitFunction(u16 DeviceId, XGpio *GpioInstancePtr)
{
	XScuGic_Config *IntcConfig;
	int status;

	// Interrupt controller initialisation
	IntcConfig = XScuGic_LookupConfig(DeviceId);
	status = XScuGic_CfgInitialize(&INTCInst, IntcConfig, IntcConfig->CpuBaseAddress);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Call to interrupt setup
	status = InterruptSystemSetup(&INTCInst);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Connect GPIO interrupt to handler
	status = XScuGic_Connect(&INTCInst,
					  	  	 INTC_GPIO_INTERRUPT_ID,
					  	  	 (Xil_ExceptionHandler)BTN_Intr_Handler,
					  	  	 (void *)GpioInstancePtr);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Enable GPIO interrupts interrupt
	XGpio_InterruptEnable(GpioInstancePtr, 1);
	XGpio_InterruptGlobalEnable(GpioInstancePtr);

	// Enable GPIO and timer interrupts in the controller
	XScuGic_Enable(&INTCInst, INTC_GPIO_INTERRUPT_ID);

	return XST_SUCCESS;
}

