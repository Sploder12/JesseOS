#ifndef SCREEN_H
#define SCREEN_H

#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0f
#define RED_ON_WHITE 0xf4

//using or with back and text can make all combos
//not specifying a component will default black
#define BLACK_BACK 0x00
#define BLUE_BACK 0x10
#define GREEN_BACK 0x20
#define TEAL_BACK 0x30
#define RED_BACK 0x40
#define PURPLE_BACK 0x50
#define ORANGE_BACK 0x60
#define GRAY_BACK 0x70
#define DGRAY_BACK 0x80
#define LBLUE_BACK 0x90
#define LGREEN_BACK 0xa0
#define LTEAL_BACK 0xb0
#define LRED_BACK 0xc0
#define MAGENTA_BACK 0xd0
#define YELLOW_BACK 0xe0
#define WHITE_BACK 0xf0

#define BLACK_TEXT 0x00
#define BLUE_TEXT 0x01
#define GREEN_TEXT 0x02
#define TEAL_TEXT 0x03
#define RED_TEXT 0x04
#define PURPLE_TEXT 0x05
#define ORANGE_TEXT 0x06
#define GRAY_TEXT 0x07
#define DGRAY_TEXT 0x08
#define LBLUE_TEXT 0x09
#define LGREEN_TEXT 0x0a
#define LTEAL_TEXT 0x0b
#define LRED_TEXT 0x0c
#define MAGENTA_TEXT 0x0d
#define YELLOW_TEXT 0x0e
#define WHITE_TEXT 0x0f

/* Screen i/o ports */
#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5

/* Public kernel API */
void clear_screen();
void kprint_color(unsigned char color);
void kprint_at(char *message, int col, int row);
void kprint(char *message);
void kprint_backspace();

#endif
