#include <stdint.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "shell.h"
#include "ch_test.h"

#define LED_EXT 14


bool watchdog_started = false;

void watchdog_callback(void) {
  palTogglePad(IOPORT1, LED2);
  palTogglePad(IOPORT1, LED3);
  palTogglePad(IOPORT1, LED4);
}

WDGConfig WDG_config = {
    .pause_on_sleep = 0,
    .pause_on_halt  = 0,
    .timeout_ms     = 5000,
    .callback       = watchdog_callback,
};


/*
 * Command Random
 */
#define RANDOM_BUFFER_SIZE 1024
static uint8_t random_buffer[RANDOM_BUFFER_SIZE];

static void cmd_random(BaseSequentialStream *chp, int argc, char *argv[]) {
    uint16_t size = 16;
    uint16_t i    = 0;
    uint8_t  nl   = 0;
    
    if (argc > 0) {
	size = atoi(argv[0]);
    }

    if (size > RANDOM_BUFFER_SIZE) {
	chprintf(chp, "random: maximum size is %d.\r\n", RANDOM_BUFFER_SIZE);
	return;
    }
    
    chprintf(chp, "Fetching %d random byte(s):\r\n", size);

    rngStart(&RNGD1, NULL);
    rngWrite(&RNGD1, random_buffer, size, TIME_INFINITE);
    rngStop(&RNGD1);

    for (i = 0 ; i < size ; i++) {
	chprintf(chp, "%02x ", random_buffer[i]);
	if ((nl = (((i+1) % 20)) == 0))
	    chprintf(chp, "\r\n");
    }
    if (!nl)
	chprintf(chp, "\r\n");
    
}




static void cmd_watchdog(BaseSequentialStream *chp, int argc, char *argv[]) {
    if ((argc == 0) || (strcmp(argv[0], "start"))) {
	chprintf(chp, "Usage: watchdog start\r\n");
	return;
    }
    chprintf(chp,
	     "Watchdog started\r\n"
	     "You need to push BTN1 every 5 seconds\r\n");

    watchdog_started = true;
    wdgStart(&WDGD1, &WDG_config);
}







static THD_WORKING_AREA(shell_wa, 1024);

static const ShellCommand commands[] = {
  {"random",   cmd_random   },
  {"watchdog", cmd_watchdog },
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SD1,
  commands
};

static SerialConfig serial_config = {
    .speed   = 115200,
    .tx_pad  = UART_TX,
    .rx_pad  = UART_RX,
#if NRF5_SERIAL_USE_HWFLOWCTRL	== TRUE
    .rts_pad = UART_RTS,
    .cts_pad = UART_CTS,
#endif
};







static THD_WORKING_AREA(waThread1, 64);
static THD_FUNCTION(Thread1, arg) {

    (void)arg;
    uint8_t led = LED4;
    
    chRegSetThreadName("blinker");

    
    while (1) {
	palSetPad(IOPORT1, led);
	chThdSleepMilliseconds(100);
	palClearPad(IOPORT1, led);
	chThdSleepMilliseconds(100);
    }
}


#define printf(fmt, ...)					\
    chprintf((BaseSequentialStream*)&SD1, fmt, ##__VA_ARGS__)




/**@brief Function for application main entry.
 */
int main(void)
{
    
    halInit();
    chSysInit();
    shellInit();
  
    sdStart(&SD1, &serial_config);

    palSetPad(IOPORT1, LED1);
    palSetPad(IOPORT1, LED2);
    palSetPad(IOPORT1, LED3);
    palSetPad(IOPORT1, LED4);

    
    
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO+1,
		      Thread1, NULL);


    
    chThdCreateStatic(shell_wa, sizeof(shell_wa), NORMALPRIO+1,
		      shellThread, (void *)&shell_cfg1);


    
    printf(PORT_INFO "\r\n");
    chThdSleep(2);
    


    printf("Priority levels %d\r\n", CORTEX_PRIORITY_LEVELS);
    
    test_execute((BaseSequentialStream *)&SD1);
    
    while (true) {
	if (watchdog_started &&
	    (palReadPad(IOPORT1, BTN1) == 0)) {
	    palTogglePad(IOPORT1, LED1);
	    wdgReset(&WDGD1);
	}
	chThdSleepMilliseconds(250);
    }

}

