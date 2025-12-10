#ifndef _V4L2_APP_H_
#define _V4L2_APP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include "shm.h"

void v4l2_start();

#endif