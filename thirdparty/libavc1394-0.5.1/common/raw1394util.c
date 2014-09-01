
#include <config.h>
#include "raw1394util.h"
#include <errno.h>
#include <time.h>

int cooked1394_read(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr,
                    size_t length, quadlet_t *buffer)
{
	int retval, i;
	struct timespec ts = {0, RETRY_DELAY};
	for(i=0; i<MAXTRIES; i++) {
		retval = raw1394_read(handle, node, addr, length, buffer);
		if (retval < 0 && errno == EAGAIN)
			nanosleep(&ts, NULL);
		else
			return retval;
	}
	return -1;
}

int cooked1394_write(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr,
                     size_t length, quadlet_t *data)
{
	int retval, i;
	struct timespec ts = {0, RETRY_DELAY};
	for(i=0; i<MAXTRIES; i++) {
		retval = raw1394_write(handle, node, addr, length, data);
		if (retval < 0 && errno == EAGAIN)
			nanosleep(&ts, NULL);
		else
			return retval;
	}
	return -1;
}
