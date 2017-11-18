#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <signal.h>

#include <artik_module.h>
#include <artik_loop.h>
#include <artik_cloud.h>
#include <artik_platform.h>
#include <artik_gpio.h>

//#define LAB3_TIMEOUT_MS	(1000*1000)
//#define LAB3_WRITE_PERIODIC_MS (1*1000)
#define MAX_PARAM_LEN (128)

#undef ARTIK_A710_GPIO0
#define ARTIK_A710_GPIO0 28 // this is the correct GPIO number for RED led

#undef ARTIK_A710_GPIO1
#define ARTIK_A710_GPIO1 38 // this is the correct GPIO number for BLUE led

#undef ARTIK_A530_GPIO0
#define ARTIK_A530_GPIO0 28 // this is the correct GPIO number for RED led

#undef ARTIK_A530_GPIO1
#define ARTIK_A530_GPIO1 38 // this is the correct GPIO number for BLUE led

artik_gpio_module *gpio;
enum {
	R = 0,
	B
};

struct led_gpios {
	artik_gpio_handle handle;
	artik_gpio_config config;
};
 struct led_gpios leds[3];


static artik_error toggle_led(unsigned int state);


static artik_error toggle_led(unsigned int state)
{

	artik_error ret = S_OK;
	printf("GPIO value %d \n", state);


	//Toggle LEDs
	if(state)
	{
		gpio->write(leds[R].handle, 1);	/* R */
//		gpio->write(leds[B].handle, 1);	/* B */
	}
	else
	{
		gpio->write(leds[R].handle, 0);	/* R */
//		gpio->write(leds[B].handle, 0);	/* B */
	}


	return ret;
}

/** Register for GPIO and */
static artik_error led_init(int platid)
{

	unsigned int i;
	artik_error ret = S_OK;

	unsigned int count = 2;

	gpio = (artik_gpio_module *)artik_request_api_module("gpio");  //Initializing with a gpio module

	if (platid == ARTIK520) {
		leds[0].config.id = ARTIK_A520_GPIO_XEINT0;
		leds[1].config.id = ARTIK_A520_GPIO_XEINT1;
	} else if (platid == ARTIK1020) {
		leds[0].config.id = ARTIK_A1020_GPIO_XEINT0;
		leds[1].config.id = ARTIK_A1020_GPIO_XEINT1;
	} else if (platid == ARTIK710) {
		leds[0].config.id = ARTIK_A710_GPIO0;
		leds[1].config.id = ARTIK_A710_GPIO1;
	} else {
		leds[0].config.id = ARTIK_A530_GPIO0;
		leds[1].config.id = ARTIK_A530_GPIO2;
	}

	//fprintf(stdout, "TEST: %s\n", __func__);

	/* Register GPIOs for LEDs */
	for (i = 0; i < (sizeof(leds) / sizeof(*leds)); i++) {
		ret = gpio->request(&leds[i].handle, &leds[i].config);
		if (ret != S_OK)
		{
			    //release the GPIO for LED if it is busy
		    	gpio->release(leds[i].handle);
				ret = gpio->request(&leds[i].handle, &leds[i].config);
				if (ret != S_OK) {
					fprintf(stdout, "\nLED is busy... \n");
					return ret;
			   }
		}

	}
//to test mechanism to ensure that the lights are initalized. Blink the RED leds
#if 1
	//Turn OFF LEDs
	gpio->write(leds[R].handle, 0);	/* R */
	gpio->write(leds[B].handle, 0);	/* B */

	i=5;
	while(i--)
	{
		printf("\nSet LED (28) value : 1");
		gpio->write(leds[R].handle, 1);	/* R */
		sleep(2);
		printf("\nSet LED (28) value : 0 \n");
		gpio->write(leds[R].handle, 0);	/* B */
		sleep(2);
		//gpio->write(leds[B].handle, 1);	/* R */
		//sleep(1);
		//gpio->write(leds[B].handle, 0);	/* B */
		//sleep(1);
	}
#endif

	return ret;
}

/**Before exit de-init the LED */
static artik_error led_deinit()
{
	artik_error ret = S_OK;
	int i;

	if(!gpio)
	{
		printf(" GPIO already de-inited \n" );
		return ret;
	}

		/* Release GPIOs for LEDs */
	for (i = 0; i < (sizeof(leds) / sizeof(*leds)); i++){

		ret = gpio->release(leds[i].handle);
		if (ret != 0 ) printf("\nThe GPIO release %d failed with Err code:%d\n",leds[i].handle,ret);
	}


	ret = artik_release_api_module(gpio);
	if (ret != S_OK) {
		fprintf(stderr, "TEST: %s failed, could not release module\n", __func__);
		return ret;
	}

	fprintf(stdout, "GPIO de-inited");

	return ret;
}

int main(int argc, char *argv[])
{
	int opt;
	artik_error ret = S_OK;
	bool enable_sdr = false;

	char msg_to_akc[MAX_PARAM_LEN];

	int platid = artik_get_platform();

	
	if (!artik_is_module_available(ARTIK_MODULE_CLOUD)) {
		fprintf(stdout,
			"Cloud module is not available, skipping the lab...\n");
		return -1;
	}

	if ((platid == ARTIK520) || (platid == ARTIK1020) || (platid == ARTIK710) || (platid == ARTIK530)) {

		ret = led_init(platid);

	}

	led_deinit();
	return 0;
}
