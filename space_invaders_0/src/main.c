#include <stdio.h>
#include <stdint.h>
#include "dma_test_1.h"
#include "platform.h"
#include "xparameters.h"

void print(char *str);

int main()
{
	uint32_t source_word = 0xDEADBEEF;
	uint32_t destination_word = 0x0;

    init_platform();

    print("Hello World\n\r");
    cleanup_platform();
    printf("Printing value before DMA transfer.\n\r");
    xil_printf("%x\r\n", destination_word);

    DMA_TEST_1_MasterRecvWord(XPAR_DMACONTROLLER_0_BASEADDR, (Xuint32) &source_word);
    DMA_TEST_1_MasterSendWord(XPAR_DMACONTROLLER_0_BASEADDR, (Xuint32) &destination_word);

    printf("Printing value after DMA transfer.\n\r");
    xil_printf("%x\r\n", destination_word);

//    cleanup_platform();

    return 0;
}
