#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include "disp_manager.h"
#include "drm_fourcc.h"
#include "v4l2_app.h"
#include "atomic_drm.h"
#include "shm.h"

int main()
{
    Shm_Create();

    // v4l2_start();
    // drm_start();
    // while(1);
	
	return 0;
}
