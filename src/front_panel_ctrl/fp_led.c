
#include "common.h"
#include "app_gpio.h"
#include "fp_led.h"
#include "input_to_channel.h"
#include "capture.h"
#include "app_signal.h"
#include "video.h"
#include "sysparams.h"

#define	LED_1		0
#define	LED_2		1
#define	LED_R		3
#define	LED_ON 		1
#define	LED_OFF		0

Int32 Ioctl_Led(int gpio, Int32 led, Int32 state)
{
	R_GPIO_data data = {0};

	if(led == 0) {
		data.gpio_num = 34;
		data.gpio_value = state;
		ioctl(gpio, 0x55555555, &data);
	} else if(led == 1) {
		data.gpio_num = 35;
		data.gpio_value = state;
		ioctl(gpio, 0x55555555, &data);
	} else {
		data.gpio_num = 33;
		data.gpio_value = state;
		ioctl(gpio, 0x55555555, &data);
	}

	return 0;
}

int ioctl_led_flash(int gpio, int led)
{
	Ioctl_Led(gpio, led, LED_ON);
	sleep(1);
	Ioctl_Led(gpio, led, LED_OFF);
	sleep(1);
}

void *front_panel_led1_process(void *arg)
{
	if(NULL == arg) {
		return NULL;
	}

	ENC2000_LINK_STRUCT *e = (ENC2000_LINK_STRUCT *)arg;
	int hdcp = 0;

	while(1) {
		if(get_capture_state(SIGNAL_INPUT_1)) {
			hdcp = app_get_hdcp(SIGNAL_INPUT_1);

			if(hdcp) {
				ioctl_led_flash(e->gpiofd, LED_1);
			} else {
				Ioctl_Led(e->gpiofd, LED_1, LED_ON);
			}
		} else {
			Ioctl_Led(e->gpiofd, LED_1, LED_OFF);
		}

		if(!hdcp) {
			usleep(300000);
		}
	}


	pthread_detach(pthread_self());
	pthread_exit(NULL);
	PRINTF("i exit thread\n");
	return arg;
}

void *front_panel_led2_process(void *arg)
{
	if(NULL == arg) {
		return NULL;
	}

	ENC2000_LINK_STRUCT *e = (ENC2000_LINK_STRUCT *)arg;
	int hdcp = 0;

	while(1) {
		if(get_capture_state(SIGNAL_INPUT_2)) {
			hdcp = app_get_hdcp(SIGNAL_INPUT_2);

			if(hdcp) {
				ioctl_led_flash(e->gpiofd, LED_2);
			} else {
				Ioctl_Led(e->gpiofd, LED_2, LED_ON);
			}
		} else {
			Ioctl_Led(e->gpiofd, LED_2, LED_OFF);
		}

		if(!hdcp) {
			usleep(300000);
		}
	}

	return arg;
}

void *front_panel_ledRun_process(void *arg)
{
	if(NULL == arg) {
		return NULL;
	}

	ENC2000_LINK_STRUCT *e = (ENC2000_LINK_STRUCT *)arg;
	//	int hdcp = 0;

	while(1) {
		Ioctl_Led(e->gpiofd, LED_R, LED_OFF);

		if(get_enc_state()) {
			usleep(66666);
			Ioctl_Led(e->gpiofd, LED_R, LED_ON);
			set_enc_state(0);
		}

		usleep(66666);
	}

	return arg;
}

static void init_front_panel_led(int gpiofd)
{
	Ioctl_Led(gpiofd, LED_1, LED_OFF);
	Ioctl_Led(gpiofd, LED_2, LED_OFF);
	Ioctl_Led(gpiofd, LED_R, LED_OFF);
}

int front_panel_led(ENC2000_LINK_STRUCT *enc2000_link)
{
	pthread_t p1, p2, p3;
	init_front_panel_led(enc2000_link->gpiofd);

	if(pthread_create(&p1, NULL, front_panel_led1_process, enc2000_link)) {
		printf("[front_panel_led]pthread_create failed !!!\n");
		return -1;
	}

	if(ENC2000 == sys_get_product_type()) {
		if(pthread_create(&p2, NULL, front_panel_led2_process, enc2000_link)) {
			printf("[front_panel_led]pthread_create failed !!!\n");
			return -1;
		}
	}

	if(pthread_create(&p3, NULL, front_panel_ledRun_process, enc2000_link)) {
		printf("[front_panel_led]pthread_create failed !!!\n");
		return -1;
	}

	return 0;
}

