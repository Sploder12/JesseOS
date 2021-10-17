#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>

void create_folder(char name[]);

void ls();
void cd(char dir[]);

void save_state();

void init_filesystem();

#endif
