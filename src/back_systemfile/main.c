
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lcdshow.h"
#include "weblisten.h"

int main(int argc, char **argv)
{
	lcd_show();
	app_weblisten_init();
	while(1) {
		sleep(1);
	}
	return 0;
}

