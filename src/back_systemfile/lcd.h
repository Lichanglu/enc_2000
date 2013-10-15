
#ifndef	__LCD_H__
#define	__LCD_H__

typedef enum lcd_show_line {
    LCD_FIRST_LINE		=	0,
    LCD_SECOND_LINE	=	15,
    LCD_THIRD_LINE	=	30,
    LCD_FOURTH_LINE	=	45
} lcd_line_t;
int InitLed(void);
int ClearScreen(int spi_fd);
int ShowString(int fd, char *string, int x, int y);
int ShowStringLine(int fd,char *string, lcd_line_t lines);

#endif	//__LCD_H__

