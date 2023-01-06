/*
 * form: https://blog.csdn.net/hexiaolong2009/article/details/83721242
 * api:  https://docs.nvidia.com/jetson/l4t-multimedia/group__direct__rendering__manager.html#gaad5e4605c45f9b796e3633fe574b82a8
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

struct buffer_object {
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t handle;
	uint32_t size;
	uint8_t *vaddr;
	uint32_t fb_id;
};

struct buffer_object buf;

static int modeset_create_fb(int fd, struct buffer_object *bo)
{
	struct drm_mode_create_dumb create = {};
 	struct drm_mode_map_dumb map = {};

	/* create a dumb-buffer, the pixel format is XRGB888 */
	create.width = bo->width;
	create.height = bo->height;
	create.bpp = 32;
	drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);

	/* bind the dumb-buffer to an FB object */
	bo->pitch = create.pitch;
	bo->size = create.size;
	bo->handle = create.handle;
	drmModeAddFB(fd, bo->width, bo->height, 24, 32, bo->pitch,
			   bo->handle, &bo->fb_id);

	/* map the dumb-buffer to userspace */
	map.handle = create.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);

	bo->vaddr = mmap(0, create.size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, map.offset);

	/* initialize the dumb-buffer with white-color */
	memset(bo->vaddr, 0xff, bo->size);

	return 0;
}

static void modeset_destroy_fb(int fd, struct buffer_object *bo)
{
	struct drm_mode_destroy_dumb destroy = {};

	drmModeRmFB(fd, bo->fb_id);

	munmap(bo->vaddr, bo->size);

	destroy.handle = bo->handle;
	drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}

int main(int argc, char **argv)
{
	int fd;
	drmModeConnector *conn;
	drmModeRes *res;
	uint32_t conn_id;
	uint32_t crtc_id;

	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

	//drmModeResPtr drmModeGetResources	(	int 	fd	)
	//Gets information about a DRM device's CRTCs, encoders, and connectors.

	//Gets a list of a DRM device's major resources. A DRM application typically calls this function early to identify available displays and other resources. 
	//The function does not report plane resources, though. These can be queried with drmModeGetPlaneResources.

	//Note
	//The drmModeResPtr structure's min_width, min_height, max_width, and max_height members are set to placeholder values.
	//Postcondition
	//If the call is successful, the application must free the resource information structure by calling drmModeFreeResources.
	//Parameters
	//fd	The file descriptor of an open DRM device.
	//Returns
	//A drmModeResPtr structure if successful, or NULL otherwise.
	res = drmModeGetResources(fd);
	crtc_id = res->crtcs[0];
	conn_id = res->connectors[0];

	printf("ljlj res count_crtcs: %d, count_connectors: %d, count_encoders: %d\n", \
			res->count_crtcs, res->count_connectors, res->count_encoders);
	//Gets information for a connector.

	//If connector_id is valid, fetches a drmModeConnectorPtr structure which contains information about a connector, such as available modes, connection status, connector type, and which encoder (if any) is attached.

	//Postcondition
	//If the call is successful, the application must free the connector information structure by calling drmModeFreeConnector.
	//Note
	//connector->mmWidth and connector->mmHeight are currently set to placeholder values.
	//Parameters
	//fd	The file descriptor of an open DRM device.
	//connector_id	The connector ID of the connector to be retrieved.
	//Returns
	//A drmModeConnectorPtr structure if successful, or NULL if the connector is not found or the API is out of memory.
	conn = drmModeGetConnector(fd, conn_id);
	buf.width = conn->modes[0].hdisplay;
	buf.height = conn->modes[0].vdisplay;
	printf("ljlj conn->tpye: %d, conn->count_modes\n", conn->count_modes);
	printf("ljlj buf.width: %d, buf.height: %d\n", buf.width, buf.height);

	modeset_create_fb(fd, &buf);

	drmModeSetCrtc(fd, crtc_id, buf.fb_id,
			0, 0, &conn_id, 1, &conn->modes[0]);

	getchar();

	modeset_destroy_fb(fd, &buf);

	drmModeFreeConnector(conn);
	drmModeFreeResources(res);

	close(fd);

	return 0;
}
