/*
 * disk_from_file.h
 *
 *  Created on: May 13, 2017
 *      Author: igor
 */

#ifndef DISK_FROM_FILE_H_
#define DISK_FROM_FILE_H_

#include "disk.h"

uint8_t DiskFromFile_Init(Disk *disk, const char *filename);

#endif /* DISK_FROM_FILE_H_ */
