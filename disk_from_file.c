/*
 * disk_from_file.c
 *
 *  Created on: May 13, 2017
 *      Author: igor
 */

#include <stdio.h>

#include "disk_from_file.h"

#define FILE(interface)     ((FILE *) interface)

static size_t DiskFromFile_Read(const Disk *disk, size_t address, void *data, size_t length) {
    //TODO handle errors
    fseek(FILE(disk->interface), address, SEEK_SET);
    return fread(data, 1, length, FILE(disk->interface));
}

uint8_t DiskFromFile_Init(Disk *disk, const char *filename) {
    //TODO handle errors
    disk->read = DiskFromFile_Read;
    disk->interface = fopen(filename, "r");
    fseek(FILE(disk->interface), 0, SEEK_END);
    //TODO handle >4GB files
    disk->size = ftell(FILE(disk->interface));
    return 0;
}

