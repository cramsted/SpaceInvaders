#include "xgpio.h"          // Provides access to PB GPIO driver.
#include <stdio.h>          // xil_printf and so forth.
#include "platform.h"       // Enables caching and other system stuff.
#include "mb_interface.h"   // provides the microblaze interrupt enables, etc.
#include "xintc_l.h"        // Provides handy macros for the interrupt controller.
#define ONE_SECOND_COUNT 100
#define HALF_SECOND_COUNT 50
#define DEBOUNCE_TIME 5
#define SECONDS_BUTTON 0x02
#define MINS_BUTTON 0x01
#define HOURS_BUTTON 0x08
#define INCREMENT_BUTTON 0x10
#define DECREMENT_BUTTON 0x04
#define MINS_SECONDS_MAX 59
#define HOURS_MAX 23
#define PRINT_TIME 	xil_printf("%02d:%02d:%02d\r",hours,mins,seconds)
#define INCREMENT(buttonState) (buttonState & INCREMENT_BUTTON)
#define DECREMENT(buttonState) (buttonState & DECREMENT_BUTTON)

XGpio gpLED; // This is a handle for the LED GPIO block.
XGpio gpPB; // This is a handle for the push-button GPIO block.

void increment_seconds(int rollover);
void increment_mins(int rolloever);
void increment_hours();
void decrement_seconds(int rollover);
void decrement_mins(int rolloever);
void decrement_hours();
void modify_time(u32 timeButton);

static int hours = 0;
static int mins = 0;
static int seconds = 0;

static int debounceCounter = 0;
static int inc_dec_hold_counter = 0;
static u32 currentButtonState;
// This is invoked in response to a timer interrupt.
// It does 2 things: 1) debounce switches, and 2) advances the time.
void timer_interrupt_handler() {
	static int counter = 0;
	counter++;

	if (debounceCounter && (--debounceCounter == 0)) {
		modify_time(currentButtonState);
	}
	if (currentButtonState & (SECONDS_BUTTON | MINS_BUTTON | HOURS_BUTTON)) {
		if (currentButtonState & (INCREMENT_BUTTON | DECREMENT_BUTTON)) {
			inc_dec_hold_counter++;
			if (inc_dec_hold_counter >= ONE_SECOND_COUNT) {
				// update time
				modify_time(currentButtonState);
				inc_dec_hold_counter = HALF_SECOND_COUNT;
			}
		}else{
			inc_dec_hold_counter = 0;
		}
	} else if (counter == ONE_SECOND_COUNT) {
		increment_seconds(1);
		PRINT_TIME;
		counter = 0;
	}
}

void modify_time(u32 timeButton) {
	if (timeButton & SECONDS_BUTTON) {
		if (INCREMENT(timeButton)) {
			increment_seconds(0);
			PRINT_TIME;
		} else if (DECREMENT(timeButton)) {
			decrement_seconds(0);
			PRINT_TIME;
		}
	}
	if (timeButton & MINS_BUTTON) {
		if (INCREMENT(timeButton)) {
			increment_mins(0);
			PRINT_TIME;
		} else if (DECREMENT(timeButton)) {
			decrement_mins(0);
			PRINT_TIME;
		}
	}
	if (timeButton & HOURS_BUTTON) {
		if (INCREMENT(timeButton)) {
			increment_hours();
			PRINT_TIME;
		} else if (DECREMENT(timeButton)) {
			decrement_hours();
			PRINT_TIME;
		}
	}
}

void test() {
	if (currentButtonState & (SECONDS_BUTTON)) {
		xil_printf("seconds\n\r");
	}
	if (currentButtonState & (MINS_BUTTON)) {
		xil_printf("mins\n\r");
	}
	if (currentButtonState & (HOURS_BUTTON)) {
		xil_printf("hours\n\r");
	}
	if (currentButtonState & (INCREMENT_BUTTON)) {
		xil_printf("increment\n\r");
	}
	if (currentButtonState & (DECREMENT_BUTTON)) {
		xil_printf("decrement\n\r");
	}
}
void increment_seconds(int rollover) {
	seconds++;
	if (seconds > MINS_SECONDS_MAX) {
		seconds = 0;
		if (rollover) {
			increment_mins(rollover);
		}
	}

}

void increment_mins(int rollover) {
	mins++;
	if (mins > MINS_SECONDS_MAX) {
		mins = 0;
		if (rollover) {
			increment_hours();
		}
	}
}
void increment_hours() {
	hours++;
	if (hours > HOURS_MAX) {
		hours = 0;
	}
}

void decrement_seconds(int rollover) {
	seconds--;
	if (seconds < 0) {
		seconds = MINS_SECONDS_MAX;
		if (rollover) {
			increment_mins(rollover);
		}
	}

}

void decrement_mins(int rollover) {
	mins--;
	if (mins < 0) {
		mins = MINS_SECONDS_MAX;
		if (rollover) {
			increment_hours();
		}
	}
}
void decrement_hours() {
	hours--;
	if (hours < 0) {
		hours = HOURS_MAX;
	}
}
// This is invoked each time there is a change in the button state (result of a push or a bounce).
void pb_interrupt_handler() {
	// Clear the GPIO interrupt.
	XGpio_InterruptGlobalDisable(&gpPB); // Turn off all PB interrupts for now.
	currentButtonState = XGpio_DiscreteRead(&gpPB, 1); // Get the current state of the buttons.
	// You need to do something here.
	debounceCounter = DEBOUNCE_TIME;

	XGpio_InterruptClear(&gpPB, 0xFFFFFFFF); // Ack the PB interrupt.
	XGpio_InterruptGlobalEnable(&gpPB); // Re-enable PB interrupts.
}

// Main interrupt handler, queries the interrupt controller to see what peripheral
// fired the interrupt and then dispatches the corresponding interrupt handler.
// This routine acks the interrupt at the controller level but the peripheral
// interrupt must be ack'd by the dispatched interrupt handler.
// Question: Why is the timer_interrupt_handler() called after ack'ing the interrupt controller
// but pb_interrupt_handler() is called before ack'ing the interrupt controller?
void interrupt_handler_dispatcher(void* ptr) {
	int intc_status = XIntc_GetIntrStatus(XPAR_INTC_0_BASEADDR);
	// Check the FIT interrupt first.
	if (intc_status & XPAR_FIT_TIMER_0_INTERRUPT_MASK) {
		XIntc_AckIntr(XPAR_INTC_0_BASEADDR, XPAR_FIT_TIMER_0_INTERRUPT_MASK);
		timer_interrupt_handler();
	}
	// Check the push buttons.
	if (intc_status & XPAR_PUSH_BUTTONS_5BITS_IP2INTC_IRPT_MASK) {
		pb_interrupt_handler();
		XIntc_AckIntr(XPAR_INTC_0_BASEADDR, XPAR_PUSH_BUTTONS_5BITS_IP2INTC_IRPT_MASK);
	}
}

//int main(void) {
//	init_platform();
//	// Initialize the GPIO peripherals.
//	int success;
//	print("hello world\n\r");
//	success = XGpio_Initialize(&gpPB, XPAR_PUSH_BUTTONS_5BITS_DEVICE_ID);
//	// Set the push button peripheral to be inputs.
//	XGpio_SetDataDirection(&gpPB, 1, 0x0000001F);
//	// Enable the global GPIO interrupt for push buttons.
//	XGpio_InterruptGlobalEnable(&gpPB);
//	// Enable all interrupts in the push button peripheral.
//	XGpio_InterruptEnable(&gpPB, 0xFFFFFFFF);
//
//	microblaze_register_handler(interrupt_handler_dispatcher, NULL);
//	XIntc_EnableIntr(XPAR_INTC_0_BASEADDR,
//			(XPAR_FIT_TIMER_0_INTERRUPT_MASK | XPAR_PUSH_BUTTONS_5BITS_IP2INTC_IRPT_MASK));
//	XIntc_MasterEnable(XPAR_INTC_0_BASEADDR);
//	microblaze_enable_interrupts();
//
//	while (1)
//		; // Program never ends.
//
//	cleanup_platform();
//
//	return 0;
//}
