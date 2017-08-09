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

#define LAB3_TIMEOUT_MS	(100*1000)
//#define LAB3_WRITE_PERIODIC_MS (1*1000)
#define MAX_PARAM_LEN (128)

#undef ARTIK_A710_GPIO4
#define ARTIK_A710_GPIO4 30

enum {
	R = 0,
	B
};
struct led_gpios {
	artik_gpio_handle handle;
	artik_gpio_config config;
};
struct led_gpios leds[3];


artik_gpio_module *gpio;
artik_gpio_handle button;


static void button_event(void *param, int value)
{
	static int button_state;

	fprintf(stdout, "Button event: %d\n", button_state);

	if(button_state)
	{
		button_state =0;
		gpio->write(leds[R].handle, button_state);	/* R */
	}
	else
	{
		button_state =1;
		gpio->write(leds[R].handle, button_state);	/* R */
	}


}


static artik_error button_deinit()
{
	artik_error ret = S_OK;
	int i;

	if(!gpio)
	{
		printf(" GPIO already de-inited \n" );
		return ret;
	}

	gpio->unset_change_callback(button);
	gpio->release(button);

	fprintf(stdout, "TEST: %s %s\n", __func__, ret == S_OK ? "succeeded" : "failed");

	artik_release_api_module(gpio);

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

	return ret;
}

static artik_error button_init(int platid)
{
	gpio = (artik_gpio_module *)artik_request_api_module("gpio");
	artik_loop_module *loop = (artik_loop_module *)artik_request_api_module("loop");
	artik_error ret = S_OK;
//	artik_gpio_handle button;
	artik_gpio_config config;

	if (platid == ARTIK520)
		config.id = ARTIK_A520_GPIO_XEINT3;
	else if (platid == ARTIK1020)
		config.id = ARTIK_A1020_GPIO_XEINT3;
	else if (platid == ARTIK710)
		config.id = ARTIK_A710_GPIO4;
	else
		config.id = ARTIK_A530_GPIO4;

	config.name = "button";
	config.dir = GPIO_IN;
	config.edge = GPIO_EDGE_RISING;
	config.initial_value = 0;

	fprintf(stdout, "TEST: %s\n", __func__);

	ret = gpio->request(&button, &config);
	if (ret != S_OK) {
		    //release the GPIO for Button if it is busy and retry
		gpio->release(button);
		ret = gpio->request(&button, &config);
		if (ret != S_OK) {
			fprintf(stdout, "\nButton is busy... \n");
			return ret;
	   }
	}

	printf("Button initialized, please press the button SW403\n");

	ret = gpio->set_change_callback(button, button_event, (void *)button);
	if (ret != S_OK) {
		fprintf(stderr, "TEST: %s failed, could not set GPIO change callback (%d)\n", __func__, ret);

	}

	loop->run(); //never gets out of this event unless the user press ctrl+c
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
	for (i = 0; i < (sizeof(leds) / sizeof(*leds)); i++)
		gpio->release(leds[i].handle);

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
		ret = button_init(platid);
	}

	led_deinit();
	button_deinit();

	return 0;
}
