#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"
#include "tc_.h"
#include "rtt_.h"
#include "rtc_.h"

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
	
	calendar rtc_initial = {2018, 3, 19, 12, 15, 45 ,1};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN);

	/* configura alarme do RTC */
	rtc_set_date_alarm(RTC, 1, rtc_initial.month, 1, rtc_initial.day);
	rtc_set_time_alarm(RTC, 1, rtc_initial.hour, 1, rtc_initial.minute, 1, rtc_initial.seccond + 20);
}

int main (void)
{
  init();

  gfx_mono_draw_filled_circle(20, 16, 16, GFX_PIXEL_SET, GFX_WHOLE);
  gfx_mono_draw_string("mundo", 50,16, &sysfont);

  f_rtt_alarme = true;
  
  while(1) {
	  
	if(flag_tc) {
		
		for (int i = 0; i < 1; i++){
			pio_clear(LED1_PIO, LED1_PIO_IDX_MASK);
			delay_ms(10);
			pio_set(LED1_PIO, LED1_PIO_IDX_MASK);
			delay_ms(10);
		}
		
		flag_tc = 0;
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
			pio_clear(LED3_PIO, LED3_PIO_IDX_MASK);
			delay_ms(200);
			pio_set(LED3_PIO, LED3_PIO_IDX_MASK);
			delay_ms(200);
		}
		
		flag_rtc = 0;
	}
	
	pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
  }
}
