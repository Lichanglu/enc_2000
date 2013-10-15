#ifndef _APP_GPIO_H__
#define _APP_GPIO_H__

typedef struct _R_GPIO_data_ {
	unsigned int gpio_num;
	unsigned int gpio_value;
} R_GPIO_data;


#define SET_GPIO (0x55555555)
#define GET_GPIO (0xAAAAAAAA)


#define GPIO_4			4
#define GPIO_5			5
#define GPIO_6			6
#define GPIO_7			7
#define GPIO_8			8
#define GPIO_9			9


#endif

