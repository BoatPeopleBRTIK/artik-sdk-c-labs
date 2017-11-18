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

#undef ARTIK_A530_GPIO4
#define ARTIK_A530_GPIO4 30

static char user_token[MAX_PARAM_LEN]="--ARTIK_user_token--";
static char *user_id = "--ARTIK_user_id--";

static char akc_switch_dtid[MAX_PARAM_LEN]="--ARTIK_Cloude_Switch_device_Type_ID--";
static char akc_switch_device_id[MAX_PARAM_LEN]="--ARTIK_Cloud_Switch_device_ID--";//switch id

static char message_body[MAX_PARAM_LEN];

artik_gpio_module *gpio;
artik_gpio_handle button;
artik_ssl_config ssl_config = {0};

static artik_error send_cloud_message(const char *t, const char *did,
					  const char *msg)
{
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	artik_error ret = S_OK;
	char *response = NULL;

	fprintf(stdout, "TEST: %s starting\n", __func__);

	ret = cloud->send_message(t, did, msg, &response, &ssl_config);

	if (response) {
		fprintf(stdout, "TEST: %s response data: %s\n", __func__,
			response);
		free(response);
	}

	if (ret != S_OK) {
		fprintf(stdout, "TEST: %s failed (err=%d)\n", __func__, ret);
		return ret;
	}

	fprintf(stdout, "TEST: %s succeeded\n", __func__);

	artik_release_api_module(cloud);

	return ret;
}

static void button_event(void *param, int value)
{
	static int button_state;

	fprintf(stdout, "Button event: %d\n", button_state);

	if(button_state)
	{
		snprintf(message_body,MAX_PARAM_LEN,"{ \"state\" : true }");
		send_cloud_message(user_token, akc_switch_device_id, message_body);
		button_state =0;
	}
	else
	{
		snprintf(message_body,MAX_PARAM_LEN,"{ \"state\" : false }");
		send_cloud_message(user_token, akc_switch_device_id, message_body);
		button_state =1;

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


	ret = gpio->set_change_callback(button, button_event, (void *)button);
	if (ret != S_OK) {
		fprintf(stderr, "TEST: %s failed, could not set GPIO change callback (%d)\n", __func__, ret);

	}

	loop->run(); //never gets out of this event unless the user press ctrl+c
	return ret;
}


int main(int argc, char *argv[])
{
	int opt;
	artik_error ret = S_OK;

	char msg_to_akc[MAX_PARAM_LEN];

	int platid = artik_get_platform();


	if (!artik_is_module_available(ARTIK_MODULE_CLOUD)) {
		fprintf(stdout,
			"Cloud module is not available, skipping the lab...\n");
		return -1;
	}

	if ((platid == ARTIK520) || (platid == ARTIK1020) || (platid == ARTIK710) || (platid == ARTIK530)) {
		ret = button_init(platid);
	}

	button_deinit();
	return 0;
}
