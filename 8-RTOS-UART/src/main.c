/*
* Example RTOS Atmel Studio
*/

#include <asf.h>
#include "conf_board.h"
#include <string.h>

#define TASK_MONITOR_STACK_SIZE            (2048/sizeof(portSTACK_TYPE))
#define TASK_MONITOR_STACK_PRIORITY        (tskIDLE_PRIORITY)
#define TASK_LED_STACK_SIZE                (1024/sizeof(portSTACK_TYPE))
#define TASK_LED_STACK_PRIORITY            (tskIDLE_PRIORITY)
#define TASK_LED1_STACK_SIZE (1024/sizeof(portSTACK_TYPE))
#define TASK_LED1_STACK_PRIORITY (tskIDLE_PRIORITY)
#define TASK_LED2_STACK_SIZE (1024/sizeof(portSTACK_TYPE))
#define TASK_LED2_STACK_PRIORITY (tskIDLE_PRIORITY)
#define TASK_LED3_STACK_SIZE (1024/sizeof(portSTACK_TYPE))
#define TASK_LED3_STACK_PRIORITY (tskIDLE_PRIORITY)
#define TASK_UART_STACK_SIZE (1024/sizeof(portSTACK_TYPE))
#define TASK_UART_STACK_PRIORITY (tskIDLE_PRIORITY)
#define TASK_EXECUTE_STACK_SIZE (1024/sizeof(portSTACK_TYPE))
#define TASK_EXECUTE_STACK_PRIORITY (tskIDLE_PRIORITY)

// led1
#define LED1_PIO           PIOA
#define LED1_PIO_ID        ID_PIOA
#define LED1_PIO_IDX       0
#define LED1_PIO_IDX_MASK  (1u << LED1_PIO_IDX)

// led3
#define LED3_PIO           PIOB
#define LED3_PIO_ID        ID_PIOB
#define LED3_PIO_IDX       2
#define LED3_PIO_IDX_MASK  (1u << LED3_PIO_IDX)

QueueHandle_t xQueueButId;
QueueHandle_t xQueueCommand;
QueueHandle_t xQueueCommandTask1;
QueueHandle_t xQueueCommandTask3;

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);
void pin_toggle(Pio *pio, uint32_t mask);

static void USART1_init(void){
	/* Configura USART1 Pinos */
	sysclk_enable_peripheral_clock(ID_PIOB);
	sysclk_enable_peripheral_clock(ID_PIOA);
	pio_set_peripheral(PIOB, PIO_PERIPH_D, PIO_PB4); // RX
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA21); // TX
	MATRIX->CCFG_SYSIO |= CCFG_SYSIO_SYSIO4;

	/* Configura opcoes USART */
	const sam_usart_opt_t usart_settings = {
		.baudrate       = 115200,
		.char_length    = US_MR_CHRL_8_BIT,
		.parity_type    = US_MR_PAR_NO,
		.stop_bits    = US_MR_NBSTOP_1_BIT    ,
		.channel_mode   = US_MR_CHMODE_NORMAL
	};

	/* Ativa Clock periferico USART0 */
	sysclk_enable_peripheral_clock(ID_USART1);

	stdio_serial_init(CONF_UART, &usart_settings);

	/* Enable the receiver and transmitter. */
	usart_enable_tx(USART1);
	usart_enable_rx(USART1);

	/* map printf to usart */
	ptr_put = (int (*)(void volatile*,char))&usart_serial_putchar;
	ptr_get = (void (*)(void volatile*,char*))&usart_serial_getchar;

	/* ativando interrupcao */
	usart_enable_interrupt(USART1, US_IER_RXRDY);
	NVIC_SetPriority(ID_USART1, 4);
	NVIC_EnableIRQ(ID_USART1);
}

void USART1_Handler(void){
	uint32_t ret = usart_get_status(USART1);

	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	char c;

	// Verifica por qual motivo entrou na interrupçcao?
	// RXRDY ou TXRDY

	//  Dados disponível para leitura
	if(ret & US_IER_RXRDY){
		usart_serial_getchar(USART1, &c);
		xQueueSendFromISR(xQueueButId, &c, 0);
		//printf("%c", c);

		// -  Transmissoa finalizada
		} else if(ret & US_IER_TXRDY){

	}
}

uint32_t usart1_puts(uint8_t *pstring){
	uint32_t i ;

	while(*(pstring + i))
	if(uart_is_tx_empty(USART1))
	usart_serial_putchar(USART1, *(pstring+i++));
}

/**
* \brief Called if stack overflow during execution
*/
extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
signed char *pcTaskName)
{
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	/* If the parameters have been corrupted then inspect pxCurrentTCB to
	* identify which task has overflowed its stack.
	*/
	for (;;) {
	}
}

void pin_toggle(Pio *pio, uint32_t mask){
	if(pio_get_output_data_status(pio, mask)) {
		pio_clear(pio, mask);
		vTaskDelay(700 / portTICK_PERIOD_MS);
	}
	else {
		pio_set(pio,mask);
		vTaskDelay(700 / portTICK_PERIOD_MS);
	}
}

/**
* \brief This function is called by FreeRTOS idle task
*/
extern void vApplicationIdleHook(void)
{
	pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
}

/**
* \brief This function is called by FreeRTOS each tick
*/
extern void vApplicationTickHook(void)
{
}

extern void vApplicationMallocFailedHook(void)
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

	/* Force an assert. */
	configASSERT( ( volatile void * ) NULL );
}

/**
* \brief This task, when activated, send every ten seconds on debug UART
* the whole report of free heap and total tasks status
*/
static void task_monitor(void *pvParameters)
{
	static portCHAR szList[256];
	UNUSED(pvParameters);
	
	const TickType_t xDelay = 3000 / portTICK_PERIOD_MS;
	
	for (;;) {
		printf("--- task ## %u\n", (unsigned int)uxTaskGetNumberOfTasks());
		vTaskList((signed portCHAR *)szList);
		printf(szList);
		vTaskDelay(xDelay);
	}
}

static void task_uartRX(void *pvParameters) {
	char buffer[20] = {0};
	int counter = 0;
	char id;

	for (;;) {
		printf("aaaaaaa\n");
		// aguarda por até 500 ms pelo se for liberado entra no if
		if(xQueueReceiveFromISR( xQueueButId, &id, ( TickType_t ) 100 / portTICK_PERIOD_MS)) {
			//printf("%c", id);
			if (id == '\n') {
				buffer[counter] = 0;
				counter = 0;
				printf("%s\n", buffer);
				xQueueSendFromISR(xQueueCommand, &buffer, 0);
			}
			
			else {
				buffer[counter] = id;
				counter ++;
			}
		}
	}
}

static void task_execute(void *pvParameters) {
	
	char buffer[20] = {0};
	xQueueCommand = xQueueCreate(32, sizeof(char[20]) );
	xQueueCommandTask1 = xQueueCreate(32, sizeof(char[10]));
	xQueueCommandTask3 = xQueueCreate(32, sizeof(char[10]));

	// verifica se fila foi criada corretamente
	if (xQueueCommand == NULL) printf("falha em criar a fila \n");
	
	// verifica se fila foi criada corretamente
	if (xQueueCommandTask1 == NULL) printf("falha em criar a fila \n");
	
	// verifica se fila foi criada corretamente
	if (xQueueCommandTask3 == NULL) printf("falha em criar a fila \n");
	
	for(;;) {
		if(xQueueReceiveFromISR( xQueueCommand, &buffer, ( TickType_t ) 100 / portTICK_PERIOD_MS)) {
			
			printf("%s\n", buffer);
			if (!strcmp("led 3 toggle", buffer)) {
				char command[10] = "toggle";
				xQueueSendFromISR(xQueueCommandTask3, &command, 0);
			}
			
			else if (!strcmp("led 3 off", buffer)) {
				char command[10] = "off";
				xQueueSendFromISR(xQueueCommandTask3, &command, 0);
			}
			
			else if (!strcmp("led 3 on", buffer)) {
				char command[10] = "on";
				xQueueSendFromISR(xQueueCommandTask3, &command, 0);
			}
			
			else if (!strcmp("led 1 toggle", buffer)) {
				char command[10] = "toggle";
				xQueueSendFromISR(xQueueCommandTask1, &command, 0);
			}
			
			else if (!strcmp("led 1 off", buffer)) {
				char command[10] = "off";
				xQueueSendFromISR(xQueueCommandTask1, &command, 0);
			}
			
			else if (!strcmp("led 1 on", buffer)) {
				char command[10] = "on";
				xQueueSendFromISR(xQueueCommandTask1, &command, 0);
			}
		}
	}
}

/**
* \brief This task, when activated, make LED blink at a fixed rate
*/
static void task_led1(void *pvParameters)
{
	pmc_enable_periph_clk(LED1_PIO_ID);
	pio_configure(LED1_PIO, PIO_OUTPUT_0, LED1_PIO_IDX_MASK, PIO_DEFAULT);
	pio_set(LED1_PIO, LED1_PIO_IDX_MASK);

	char command[10];
	
	for (;;) {
		if (xQueueReceiveFromISR(xQueueCommandTask1, &command, (TickType_t) 100 / portTICK_PERIOD_MS)) {
			if (!strcmp("toggle", command)) pin_toggle(LED1_PIO, LED1_PIO_IDX_MASK);
		}
	}
}

/**
* \brief This task, when activated, make LED blink at a fixed rate
*/
static void task_led3(void *pvParameters)
{
	pmc_enable_periph_clk(LED3_PIO_ID);
	pio_configure(LED3_PIO, PIO_OUTPUT_0, LED3_PIO_IDX_MASK, PIO_DEFAULT);
	pio_set(LED3_PIO, LED3_PIO_IDX_MASK);
	
	char command[10];
	
	for (;;) {
		if (xQueueReceiveFromISR(xQueueCommandTask3, &command, (TickType_t) 100 / portTICK_PERIOD_MS)) {
			if (!strcmp("toggle", command)) pin_toggle(LED3_PIO, LED3_PIO_IDX_MASK);
		}
	}
}

/**
*  \brief FreeRTOS Real Time Kernel example entry point.
*
*  \return Unused (ANSI-C compatibility).
*/
int main(void)
{
	/* Initialize the SAM system */
	printf("aaaaaaa\n");
	sysclk_init();
	board_init();
printf("aaaaaaa\n");
	// cria fila de 32 slots de char
	xQueueButId = xQueueCreate(32, sizeof(char) );

	// verifica se fila foi criada corretamente
	if (xQueueButId == NULL) printf("falha em criar a fila \n");

	/* Initialize the console uart */
	USART1_init();
	
	/* Output demo information. */
	printf("-- Freertos Example --\n\r");
	printf("-- %s\n\r", BOARD_NAME);
	printf("-- Compiled: %s %s --\n\r", __DATE__, __TIME__);

	/* Create task to monitor processor activity */
	if (xTaskCreate(task_monitor, "Monitor", TASK_MONITOR_STACK_SIZE, NULL, TASK_MONITOR_STACK_PRIORITY, NULL) != pdPASS) printf("Failed to create Monitor task\r\n");

	/* Create task to make led blink */
	if (xTaskCreate(task_led1, "Led1", TASK_LED1_STACK_SIZE, NULL, TASK_LED1_STACK_PRIORITY, NULL) != pdPASS) printf("Failed to create test led task\r\n");
	if (xTaskCreate(task_led3, "Led3", TASK_LED3_STACK_SIZE, NULL, TASK_LED3_STACK_PRIORITY, NULL) != pdPASS) printf("Failed to create test led task\r\n");
	if (xTaskCreate(task_uartRX, "Task Uart", TASK_UART_STACK_SIZE, NULL, TASK_UART_STACK_PRIORITY, NULL) != pdPASS) printf("Failed to create test uart task\r\n");
	if (xTaskCreate(task_execute, "Task execute", TASK_EXECUTE_STACK_SIZE, NULL, TASK_EXECUTE_STACK_PRIORITY, NULL) != pdPASS) printf("Failed to create test execute task\r\n");
	
	/* Start the scheduler. */
	vTaskStartScheduler();

	/* Will only get here if there was insufficient memory to create the idle task. */
	return 0;
}
