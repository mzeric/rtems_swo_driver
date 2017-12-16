/*
 *  COPYRIGHT (c) 1989-2009.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <bsp.h>
#include <fcntl.h>
#include <rtems/bspIo.h>
#include <rtems/libio.h>

#include "led.h"

volatile int led_do_print;
volatile int led_value;
rtems_id     Timer1;
rtems_id     Timer2;



rtems_device_driver swo_init( rtems_device_major_number major,
		rtems_device_minor_number minor,
		void *arg){

	//printk("sow dirver init\n");
	return RTEMS_SUCCESSFUL;
}
rtems_device_driver swo_open( rtems_device_major_number major,
		rtems_device_minor_number minor,
		void *arg){

	//printk("swo dirver open\n");
	return RTEMS_SUCCESSFUL;
}
rtems_device_driver swo_close( rtems_device_major_number major,
		rtems_device_minor_number minor,
		void *arg){

	//printk("swo dirver close\n");
	return RTEMS_SUCCESSFUL;
}
rtems_device_driver swo_read( rtems_device_major_number major,
		rtems_device_minor_number minor,
		void *arg){

	//printk("swo dirver read\n");
	return RTEMS_SUCCESSFUL;
}
#define ITM_Port8(n)    (*((volatile unsigned char *)(0xE0000000+4*n)))
#define ITM_Port16(n)   (*((volatile unsigned short*)(0xE0000000+4*n)))
#define ITM_Port32(n)   (*((volatile unsigned long *)(0xE0000000+4*n)))

#define DEMCR           (*((volatile unsigned long *)(0xE000EDFC)))
#define TRCENA          0x01000000

int itm_putc(int ch) {
	if (DEMCR & TRCENA) {
		while (ITM_Port32(0) == 0);
		ITM_Port8(0) = ch;
	}
	return(ch);
}
rtems_device_driver swo_write( rtems_device_major_number major,
		rtems_device_minor_number minor,
		void *arg){

	rtems_libio_rw_args_t * rw_args = (rtems_libio_rw_args_t*) arg;
	//printk("swo dirver write\n");
	for(int i = 0; i < rw_args->count; ++i)
		itm_putc(((char*)rw_args->buffer)[i]);
	return RTEMS_SUCCESSFUL;
}
rtems_device_driver swo_ctrl( rtems_device_major_number major,
		rtems_device_minor_number minor,
		void *arg){

	printk("dirver ctrl\n");
	return RTEMS_SUCCESSFUL;
}
#define SWO_DRIVER_TABLE_ENTRY \
  { swo_init, swo_open, swo_close, \
    swo_read, swo_write, swo_ctrl }
rtems_driver_address_table rtems_swo_io_ops = {
  initialization_entry: swo_init,
  open_entry:           swo_open,
  close_entry:          swo_close,
  read_entry:           swo_read,
  write_entry:          swo_write,
  control_entry:        swo_ctrl
};


void LED_Change_Routine( void ) {
  int _led_do_print;
  int _led_value;

  /* technically the following 4 statements are a critical section */
  _led_do_print = led_do_print;
  _led_value = led_value;
  led_do_print = 0;
  led_value = 0;

  itm_putc('+');
  
  if ( _led_do_print ) {
    if ( _led_value == 1 )
      LED_OFF();
    else
      LED_ON();
  }
}

rtems_timer_service_routine Timer_Routine( rtems_id id, void *ignored )
{
  if ( id == Timer1 )
    led_value = 1;
  else
    led_value = 2;
  led_do_print = 1;

  (void) rtems_timer_fire_after(
    id,
    2 * rtems_clock_get_ticks_per_second(),
    Timer_Routine,
    NULL
  );
}

// extern int printk(const char *fmt, ...) RTEMS_PRINTFLIKE(1, 2);
extern BSP_output_char_function_type BSP_output_char;

void setup_swo_stdout(){
	rtems_device_major_number swo_major;
	rtems_device_minor_number swo_minor;
	rtems_status_code ret;

	/* setup output for printk */
	BSP_output_char = itm_putc;

	ret = rtems_io_register_driver(0, &rtems_swo_io_ops, &swo_major);
	if (ret != RTEMS_SUCCESSFUL) {
		printk("register swo driver failed:%s\n", rtems_status_text(ret));
	}
	swo_minor = 0;
	if( (ret = rtems_io_register_name("/dev/swo", swo_major, swo_minor)) != RTEMS_SUCCESSFUL){
		printk("register /dev/swo name failed %s\n", rtems_status_text(ret));
	}
	//rtems_io_initialize(swo_major, 0, NULL);
	int fp = open("/dev/swo", O_RDWR);
	if (fp < 0){
		printk("open /dev/swo failed:%d\n", fp);
	}else{

		//printk("fp = 0x%x\n", fp);
		//write(fp, "[SWO]\n", 6);
		//close(fp);
		dup2(fp, 1);
	}
	printf("swo driver(%d, %d) initialized ok\n", swo_major, swo_minor);
}
rtems_task Init(
  rtems_task_argument argument
)
{
  setup_swo_stdout();
  printf("*** LED BLINKER -- timer *** \n");

  LED_INIT();
  printf("init led ok\n");

  (void) rtems_timer_create(rtems_build_name( 'T', 'M', 'R', '1' ), &Timer1);

  (void) rtems_timer_create(rtems_build_name( 'T', 'M', 'R', '2' ), &Timer2);

  Timer_Routine(Timer1, NULL);
  LED_Change_Routine();

  (void) rtems_task_wake_after( rtems_clock_get_ticks_per_second() );

  Timer_Routine(Timer2, NULL);
  LED_Change_Routine();

  while (1) {
    (void) rtems_task_wake_after( 10 );
    LED_Change_Routine();
  }

  (void) rtems_task_delete( RTEMS_SELF );
}
/**************** START OF CONFIGURATION INFORMATION ****************/

#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER

#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM
#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 6

#define CONFIGURE_MAXIMUM_TASKS         1
#define CONFIGURE_MAXIMUM_TIMERS        2

#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 40
#define CONFIGURE_MAXIMUM_DRIVERS 10
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT
//#define CONFIGURE_HAS_OWN_DEVICE_DRIVER_TABLE
//#define CONFIGURE_APPLICATION_EXTRA_DRIVERS  SWO_DRIVER_TABLE_ENTRY
#include <rtems/confdefs.h>
/****************  END OF CONFIGURATION INFORMATION  ****************/
