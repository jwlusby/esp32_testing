/*
 * spiffs_vfs.h
 *
 *  Created on: Dec 3, 2016
 *      Author: kolban
 */

#ifndef MAIN_SPIFFS_VFS_H_
#define MAIN_SPIFFS_VFS_H_
#include "spiffs.h"

#ifdef __cplusplus
extern "C" 
{
#endif
void spiffs_registerVFS(const char *mountPoint, spiffs *fs);


#ifdef __cplusplus
}
#endif


#endif /* MAIN_SPIFFS_VFS_H_ */
