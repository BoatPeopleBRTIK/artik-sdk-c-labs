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

#define LAB3_TIMEOUT_MS	(1000*1000)
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

//URL to use for user token and user ID   https://developer.artik.cloud/api-console/

static char user_token[MAX_PARAM_LEN]="--ARTIK_user_token--";
static char *user_id = "--ARTIK_user_id--";

static char akc_light_dtid[MAX_PARAM_LEN]="--ARTIK_Cloude_Light_device_Type_ID--"; // artik_cloud_light device type ID
static char akc_light_device_id[MAX_PARAM_LEN]="--ARTIK_Cloud_Light_device_ID--"; // delete the id

static char message_body[MAX_PARAM_LEN];

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

/**
This API is to parse the JSON output coming from AKC cloud
 */
static char *parse_json_object(const char *data, const char *obj)
{
	char *res = NULL;
	char prefix[256];
	char *substr = NULL;

	snprintf(prefix, 256, "\"%s\":\"", obj);

	substr = strstr(data, prefix);
	if (substr != NULL) {
		int idx = 0;

		/* Start after substring */
		substr += strlen(prefix);

		/* Count number of bytes to extract */
		while (substr[idx] != '\"')
			idx++;
		/* Copy the extracted string */
		res = strndup(substr, idx);
	}

	return res;
}

/** A pre determined time out callback to close the websocket */
static void on_timeout_callback(void *user_data)
{
	artik_loop_module *loop = (artik_loop_module *) user_data;

	fprintf(stdout, "LAB3: %s stop scanning, exiting loop\n", __func__);

	loop->quit();
}

/** The registered callback function to the websocket **/
void websocket_receive_callback(void *user_data, void *result)
{
	char *buffer = (char *)result;
	char *action_name=NULL;
	int i=2;


	if (buffer == NULL) {
		fprintf(stdout, "receive failed\n");
		return;
	}
//	printf("received: %s\n", buffer);

	if (buffer) {
		fprintf(stdout, "LAB3: %s response data: %s\n", __func__, buffer);
		action_name = parse_json_object(buffer, "name");

		/**Parse the JSON only if there is a action from  AKC*/
		if(action_name)
		{
			if(strcmp(action_name,"setOn")==0)
			{
				printf("\n action %s \n", action_name);
				toggle_led(1);
			}

			if(strcmp(action_name,"setOff")==0)
			{
				printf("\n This action %s \n", action_name);
				toggle_led(0);
			}

		}
	}

	/** Free the buffer, after parsing the JSON. to prevent memory leak*/
	free(result);
}

/** Open the websocket and register for the callback function to parse the recived data from AKC */
static artik_error websocket_read(int timeout_ms, artik_ssl_config *p_ssl_config)
{
	artik_error ret = S_OK;
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	artik_loop_module *loop = (artik_loop_module *)artik_request_api_module("loop");

	artik_websocket_handle handle;
	int timeout_id = 0;

	/* Open websocket to ARTIK Cloud and register device to receive messages from cloud */
	ret = cloud->websocket_open_stream(&handle, user_token, akc_light_device_id, p_ssl_config);
	if (ret != S_OK) {
		fprintf(stderr, "LAB3 failed, could not open Websocket (%d)\n", ret);
		return ret;
	}

	ret = cloud->websocket_set_receive_callback(handle, websocket_receive_callback, &handle);
	if (ret != S_OK) {
		fprintf(stderr, "LAB3 failed, could not open Websocket (%d)\n", ret);
		return ret;
	}

	ret = loop->add_timeout_callback(&timeout_id, timeout_ms, on_timeout_callback,
					   (void *)loop);

	loop->run();

	/** This part of the code is executed only after the LAB3_TIMEOUT_MS is expired **/
	cloud->websocket_close_stream(handle);

	fprintf(stdout, "LAB3: %s finished\n", __func__);

	artik_release_api_module(cloud);
	artik_release_api_module(loop);

	return ret;
}



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
#if 0
	//Turn OFF LEDs
	gpio->write(leds[R].handle, 0);	/* R */
	gpio->write(leds[B].handle, 0);	/* B */

	i=1;
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
	artik_ssl_config ssl_config = {0};

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


	websocket_read(LAB3_TIMEOUT_MS, &ssl_config); //wait in a loop until the time out expires

	led_deinit();
	return 0;
}
