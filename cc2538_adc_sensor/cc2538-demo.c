#include "contiki.h"
#include "cpu.h"
#include "sys/etimer.h"
#include "sys/rtimer.h"
#include "dev/leds.h"
#include "dev/uart.h"
#include "dev/button-sensor.h"
#include "dev/adc-sensor.h"
#include "dev/watchdog.h"
#include "dev/serial-line.h"
#include "dev/sys-ctrl.h"
#include "net/rime/broadcast.h"
#include <stdio.h>
#include <stdint.h>

/*---------------------------------------------------------------------------*/
#define LOOP_INTERVAL       CLOCK_SECOND
#define LEDS_OFF_HYSTERISIS (RTIMER_SECOND >> 1)
#define LEDS_PERIODIC       LEDS_YELLOW
#define LEDS_BUTTON         LEDS_RED
#define LEDS_SERIAL_IN      LEDS_ORANGE
#define LEDS_REBOOT         LEDS_ALL
#define LEDS_RF_RX          (LEDS_YELLOW | LEDS_ORANGE)
#define BROADCAST_CHANNEL   129
/*---------------------------------------------------------------------------*/
static struct etimer et;
static struct rtimer rt;
static uint16_t counter;
static uint16_t sleep;
static uint16_t node_ID = 0;
int Transmit_Flag = 0;
static uint16_t b[2];
int16_t Vdd;
int16_t light;
int16_t temp;
/*---------------------------------------------------------------------------*/
PROCESS(cc2538_demo_process, "cc2538 demo process");
AUTOSTART_PROCESSES(&cc2538_demo_process);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
	leds_toggle(LEDS_RF_RX);
}
/*---------------------------------------------------------------------------*/
static const struct broadcast_callbacks bc_rx = { broadcast_recv };
static struct broadcast_conn bc;
/*---------------------------------------------------------------------------*/
void
rt_callback(struct rtimer *t, void *ptr)
{
	leds_off(LEDS_PERIODIC);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cc2538_demo_process, ev, data)
{
	PROCESS_EXITHANDLER(broadcast_close(&bc))

	PROCESS_BEGIN();

	//lpm_enter();
        //REG(SYS_CTRL_PMCTL) = SYS_CTRL_PMCTL_PM2;
        //lmp_exit();
        //REG(SYS_CTRL_PMCTL) = SYS_CTRL_PMCTL_PM0;

	counter = 0;
	while(1) {

		etimer_set(&et, CLOCK_SECOND*0.5);
		PROCESS_YIELD();

		if(ev == PROCESS_EVENT_TIMER) {
			leds_on(LEDS_PERIODIC);
      		
			node_ID = 7;
			printf("Counter = 0x%08x, node_ID =%d\n\r", counter, node_ID);
			Vdd = adc_sensor.value(ADC_SENSOR_VDD_3);
        	        Vdd = ((Vdd * (3 * 1190) / (2047 << 4))/1000);
        	        printf("This is end device with node_ID=%d ", node_ID); 
			printf("transmitting value of the positive power supply to its pins,");
			printf(" 'Vdd' ='%d' volts to the Coordinator.\n\r", Vdd);
        	       	printf("\n\n");
			b[0] = node_ID;
        	        b[1] = Vdd;
	       		packetbuf_copyfrom(&b, sizeof(b));
			broadcast_open(&bc, BROADCAST_CHANNEL, &bc_rx);
			broadcast_send(&bc);
			broadcast_close(&bc);		

			
			light = adc_sensor.value(ADC_SENSOR_ALS);
			if (light <= 6000 || light >= 17500) {
				node_ID = 4;
				printf("Counter = 0x%08x, node_ID =%d\n\r", counter, node_ID);
             			printf("This is end device with node_ID=%d ", node_ID);
				printf("transmitting raw ambient light sensor value ='%d' lux to the Coordinator.\n\r", light);
               			printf("\n\n");
				b[0] = node_ID;
                		b[1] = light;
                		packetbuf_copyfrom(&b, sizeof(b));
                		broadcast_open(&bc, BROADCAST_CHANNEL, &bc_rx);
                        	broadcast_send(&bc);
                        	broadcast_close(&bc);
			}

			temp = adc_sensor.value(ADC_SENSOR_TEMP);
			temp = (((25 + ((temp >> 4) - 1422) * 10 / 42) - 3)/2);
			if (temp <= 10) {
				node_ID = 1;        	
				printf("Counter = 0x%08x, node_ID =%d\n\r", counter, node_ID);
                		printf("This is end device with node_ID=%d ", node_ID);
				printf("transmitting Temperature value ='%d' degree celsius to the Coordinator.\n\r", temp);
                		printf("\n\n");
				b[0] = node_ID;
                		b[1] = temp;
                		packetbuf_copyfrom(&b, sizeof(b));
                		broadcast_open(&bc, BROADCAST_CHANNEL, &bc_rx);
                        	broadcast_send(&bc);
                        	broadcast_close(&bc);
			}

                	rtimer_set(&rt, RTIMER_NOW() + LEDS_OFF_HYSTERISIS, 4, rt_callback, NULL);
                	Transmit_Flag=0;

			etimer_set(&et, CLOCK_SECOND*0.5);
			
    		} else if(ev == sensors_event) {
      			if(data == &button_select_sensor) {
        			packetbuf_copyfrom(&b, sizeof(b));
        			broadcast_send(&bc);
      			} else if(data == &button_left_sensor || data == &button_right_sensor) {
        			leds_toggle(LEDS_BUTTON);
      			} else if(data == &button_down_sensor) {
        		
				cpu_cpsid();
        			leds_on(LEDS_REBOOT);
        			watchdog_reboot();
      			} else if(data == &button_up_sensor) {
        			sys_ctrl_reset();
      			}
		}
    		counter++;
	}
	PROCESS_END();  
}
