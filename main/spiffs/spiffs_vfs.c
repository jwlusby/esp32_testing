#include "spiffs.h"
#include "spiffs_vfs.h"
#include <esp_vfs.h>
#include <esp_log.h>
#include <fcntl.h>
#include <errno.h>

#include "sdkconfig.h"


#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
/*
static int spiffsErrMap(spiffs *fs) {
	int errorCode = SPIFFS_errno(fs);
	switch (errorCode) {
		case SPIFFS_ERR_FULL:
			return ENOSPC;
		case SPIFFS_ERR_NOT_FOUND:
			return ENOENT;
		case SPIFFS_ERR_FILE_EXISTS:
			return EEXIST;
		case SPIFFS_ERR_NOT_A_FILE:
			return EBADF;
		case SPIFFS_ERR_OUT_OF_FILE_DESCS:
			return ENFILE;
		default: {
			ESP_LOGE(tag, "We received SPIFFs error code %d but didn't know how to map to an errno", errorCode);
			return ENOMSG;
		}
	}
} // spiffsErrMap
*/


/**
 * Log the flags that are specified in an open() call.
 */
static void logFlags(int flags) {
	static char tag[] = "logFlags";
	ESP_LOGD(tag, "flags:");
	if (flags & O_APPEND) {
		ESP_LOGD(tag, "- O_APPEND");
	}
	if (flags & O_CREAT) {
		ESP_LOGD(tag, "- O_CREAT");
	}
	if (flags & O_TRUNC) {
		ESP_LOGD(tag, "- O_TRUNC");
	}
	if (flags & O_RDONLY) {
		ESP_LOGD(tag, "- O_RDONLY");
	}
	if (flags & O_WRONLY) {
		ESP_LOGD(tag, "- O_WRONLY");
	}
	if (flags & O_RDWR) {
		ESP_LOGD(tag, "- O_RDWR");
	}
} // End of logFlags


static size_t vfs_write(void *ctx, int fd, const void *data, size_t size) {
	static char tag[] = "vfs_write";
	ESP_LOGI(tag, ">> write fd=%d, data=0x%X, size=%i", fd, (int32_t)data, (int32_t)size);
	spiffs *fs = (spiffs *)ctx;
	size_t retSize = SPIFFS_write(fs, (spiffs_file)fd, (void *)data, size);
	return retSize;
} // vfs_write


static off_t vfs_lseek(void *ctx, int fd, off_t offset, int whence) {
	static char tag[] = "vfs_lseek";
	ESP_LOGI(tag, ">> lseek fd=%d, offset=%d, whence=%d", fd, (int)offset, whence);
	spiffs *fs = (spiffs *)ctx;
	SPIFFS_lseek(fs, (spiffs_file)fd, offset, whence);

	return 0;
} // vfs_lseek


static ssize_t vfs_read(void *ctx, int fd, void *dst, size_t size) {
	static char tag[] = "vfs_read";
  ESP_LOGI(tag, ">> read ctx=%X, fd=%d, dst=0x%X, size=%i", (int32_t)ctx, fd, (int32_t)dst, (int32_t)size);
  spiffs *fs = (spiffs *)ctx;
  ssize_t retSize = SPIFFS_read(fs, (spiffs_file)fd, dst, size);
  return retSize;
} // vfs_read


/**
 * Open the file specified by path.  The flags contain the instructions
 * on how the file is to be opened.  For example:
 *
 * O_CREAT  - Create the named file.
 * O_TRUNC  - Truncate (empty) the file.
 * O_RDONLY - Open the file for reading only.
 * O_WRONLY - Open the file for writing only.
 * O_RDWR   - Open the file for reading and writing.
 * O_APPEND - Append to the file.
 *
 * The mode are access mode flags.
 */
static int vfs_open(void *ctx, char *path, int flags, int accessMode) {
	static char tag[] = "vfs_open";
	ESP_LOGI(tag, ">> open path=%s, flags=0x%x, accessMode=0x%X", path, flags, accessMode);
	logFlags(flags);
	spiffs *fs = (spiffs *)ctx;
	int spiffsFlags = 0;
    int rw = (flags & 3);

    if (rw == O_RDONLY || rw == O_RDWR) {
    	spiffsFlags |= SPIFFS_RDONLY;
    }

    if (rw == O_WRONLY || rw == O_RDWR) {
    	spiffsFlags |= SPIFFS_WRONLY;
    }

    if (flags & O_CREAT) {
    	spiffsFlags |= SPIFFS_CREAT;
    }

    if (flags & O_TRUNC) {
    	spiffsFlags |= SPIFFS_TRUNC;
    }

    if (flags & O_APPEND) {
    	spiffsFlags |= SPIFFS_APPEND;
    }
	int rc = SPIFFS_open(fs, path, spiffsFlags, accessMode);
	return rc;
} // vfs_open


static int vfs_close(void *ctx, int fd) {
	static char tag[] = "vfs_close";
	ESP_LOGI(tag, ">> close fd=%d", fd);
	spiffs *fs = (spiffs *)ctx;
	int rc = SPIFFS_close(fs, (spiffs_file)fd);
	return rc;
} // vfs_close


static int vfs_fstat(void *ctx, int fd, struct stat *st) {
	static char tag[] = "vfs_fstat";
	ESP_LOGI(tag, ">> fstat fd=%d, st=0x%X", fd, (uint32_t)st);
  spiffs *fs = (spiffs *)ctx;
	int rc = SPIFFS_fstat(fs, (spiffs_file)fd, (spiffs_stat*) st);
	return rc;
 
} // vfs_fstat


static int vfs_stat(void *ctx,  char *path, struct stat *st) {
	static char tag[] = "vfs_stat";
	ESP_LOGI(tag, "!!!NOT_IMPLEMENTED!!! >> stat path=%s", path);
	return 0;
} // vfs_stat


static int vfs_link(void *ctx,  char *oldPath,  char *newPath) {
	static char tag[] = "vfs_link";
	ESP_LOGI(tag, "!!!NOT_IMPLEMENTED!!! >> link oldPath=%s, newPath=%s", oldPath, newPath);
	return 0;
} // vfs_link


static int vfs_rename(void *ctx,  char *oldPath,  char *newPath) {
	static char tag[] = "vfs_rename";
	ESP_LOGI(tag, ">> rename oldPath=%s, newPath=%s", oldPath, newPath);
	spiffs *fs = (spiffs *)ctx;
	int rc = SPIFFS_rename(fs, oldPath, newPath);
	return rc;
} // vfs_rename


/**
 * Register the VFS at the specified mount point.
 * The callback functions are registered to handle the
 * different functions that may be requested against the
 * VFS.
 */
#ifdef __cplusplus
extern "C"
#endif
void spiffs_registerVFS(const char *mountPoint, spiffs *fs) {
	static char tag[] = "spiffs_registerVFS";
	esp_vfs_t vfs;
	esp_err_t err;

	vfs.fd_offset = 0;
	vfs.flags = ESP_VFS_FLAG_CONTEXT_PTR;
	vfs.write_p  = vfs_write;
	vfs.lseek_p  = vfs_lseek;
	vfs.read_p   = vfs_read;
	vfs.open_p   = vfs_open;
	vfs.close_p  = vfs_close;
	vfs.fstat_p  = vfs_fstat;
	vfs.stat_p   = vfs_stat;
	vfs.link_p   = vfs_link;
	vfs.rename_p = vfs_rename;

	err = esp_vfs_register(mountPoint, &vfs, (void *)fs);
	if (err != ESP_OK) {
		ESP_LOGE(tag, "esp_vfs_register: err=%i", err);
	}
 else{
    ESP_LOGI(tag, "esp_vfs_register: OK (%i)", err);
  }
} // spiffs_registerVFS
