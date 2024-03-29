#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"
#include "tc_.h"
#include "rtt_.h"
#include "rtc_.h"
#define STR_SIZE 2

void init(void) {
	
	board_init();
	sysclk_init();
	delay_init();

	// Init OLED
	gfx_mono_ssd1306_init();
	
	/* Disable the watchdog */
	WDT->WDT_MR = WDT_MR_WDDIS;
	
	pmc_enable_periph_clk(LED1_PIO_ID);
	pmc_enable_periph_clk(LED2_PIO_ID);
	pmc_enable_periph_clk(LED3_PIO_ID);
	
	pio_set_output(LED1_PIO, LED1_PIO_IDX_MASK, 0, 0, 0);
	pio_set_output(LED2_PIO, LED2_PIO_IDX_MASK, 0, 0, 0);
	pio_set_output(LED3_PIO, LED3_PIO_IDX_MASK, 0, 0, 0);
	
	pio_set(LED1_PIO, LED1_PIO_IDX_MASK);
	pio_set(LED2_PIO, LED2_PIO_IDX_MASK);
	pio_set(LED3_PIO, LED3_PIO_IDX_MASK);
	
	TC_init(TC0, ID_TC1, 1, 4);
	TC_init(TC0, ID_TC0, 0, 5);
	
	calendar rtc_initial = {2018, 3, 19, 12, 15, 45 ,1};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN | RTC_IER_SECEN);

	/* configura alarme do RTC */
	rtc_set_date_alarm(RTC, 1, rtc_initial.month, 1, rtc_initial.day);
	rtc_set_time_alarm(RTC, 1, rtc_initial.hour, 1, rtc_initial.minute, 1, rtc_initial.seccond + 20);
}

int main (void)
{
  init();
  
  uint32_t hour;
  uint32_t seconds;
  uint32_t minutes;
  
  char time[STR_SIZE];

  f_rtt_alarme = true;
  
  while(1) {
	
	rtc_get_time(RTC, &hour, &minutes, &seconds);
	
	sprintf(time, "%d:%d:%d", hour, minutes, seconds);

	gfx_mono_draw_string(time, 30, 16, &sysfont);
	
	if(flag_tc_1) {
		
		for (int i = 0; i < 1; i++){
			pio_clear(LED1_PIO, LED1_PIO_IDX_MASK);
			delay_ms(10);
			pio_set(LED1_PIO, LED1_PIO_IDX_MASK);
			delay_ms(10);
		}
		
		flag_tc_1 = 0;
	}
	
	if(flag_tc_0) {
		
		for (int i = 0; i < 1; i++){
			pio_clear(LED2_PIO, LED2_PIO_IDX_MASK);
			delay_ms(10);
			pio_set(LED2_PIO, LED2_PIO_IDX_MASK);
			delay_ms(10);
		}
		
		flag_tc_0 = 0;
	}
	
	if (f_rtt_alarme) {
		
       // IRQ apos 4s -> 8*0.5
	   uint16_t pllPreScale = (int) (((float) 32768) / 4.0);
	   uint32_t irqRTTvalue = 8;
      
		// reinicia RTT para gerar um novo IRQ
		RTT_init(pllPreScale, irqRTTvalue);         
      
		f_rtt_alarme = false;
	}
	
     if(flag_rtc) {
		
		for (int i = 0; i < 5; i++){
			pio_clear(LED_PIO, LED_PIO_IDX_MASK);
			delay_ms(200);
			pio_set(LED_PIO, LED_PIO_IDX_MASK);
			delay_ms(200);
		}
		
		flag_rtc = 0;
	}
	
	pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
  }
}