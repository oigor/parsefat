/*
 * disk.h
 *
 *  Created on: May 13, 2017
 *      Author: igor
 */

#ifndef DISK_H_
#define DISK_H_

#include <stddef.h>
#include <stdint.h>

typedef struct _Disk Disk;

typedef size_t (*Disk_read)(const Disk *disk, size_t address, void *data, size_t length);

struct _Disk {
    void *interface;
    Disk_read read;
    size_t size;
};

static inline size_t Disk_Read(const Disk *disk, size_t address, void *data, size_t length) {
    return disk->read(disk, address, data, length);
}

#endif /* DISK_H_ */
