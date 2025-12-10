#ifndef __MAIN_H
#define __MAIN_H

#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdint.h>
#include "shm.h"

void NV21_To_ARGB8888(const uint8_t* nv21, uint32_t* argb, int width, int height) ;

#endif