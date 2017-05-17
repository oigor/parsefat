/*
 * fat.h
 *
 *  Created on: May 14, 2017
 *      Author: igor
 */

#ifndef FAT_H_
#define FAT_H_

#include <wchar.h>

#include "disk.h"

//0 – Reserved Region
//1 – FAT Region
//2 – Root Directory Region (doesn’t exist on FAT32 volumes)
//3 – File and Directory Data Region

//data structures for the FAT format are all “little endian.”

typedef struct {
    uint16_t    BPB_BytsPerSec;
    uint8_t     BPB_SecPerClus;
    uint16_t    BPB_RsvdSecCnt;
    uint8_t     BPB_NumFATs;
    uint32_t    BPB_TotSec32;
    uint32_t    BPB_FATSz32;
    uint32_t    BPB_RootClus;
} BPB;

typedef struct {
    Disk    *disk;
    BPB     bpb;
} FAT;

uint8_t FAT_Init(FAT *fat, Disk *disk);

//For FAT32 volumes, each FAT entry is 32 bits in length

//FAT Entry Values
//
//0x0000000
//Cluster is free.
//
//0x0000002 to
//MAX
//Cluster is allocated. Value of the entry is the
//cluster number of the next cluster following this
//corresponding cluster. MAX is the Maximum
//Valid Cluster Number
//
//(MAX + 1) to
//0xFFFFFF6
//Reserved and must not be used.
//
//0xFFFFFF7
//Reserved and must not be used.
//
//0xFFFFFF8 to
//0xFFFFFFE
//Reserved and should not be used. May be
//interpreted as an allocated cluster and the final
//cluster in the file (indicating end-of-file
//condition).
//
//0xFFFFFFFF
//Cluster is allocated and is the final cluster for
//the file (indicates end-of-file).

typedef enum {
    CLUSTER_FREE = 0x0000000,
    CLUSTER_FINAL = 0xFFFFFFFF,
} CLUSTER;

uint32_t FAT_GetNextCluster(FAT *fat, uint32_t current_cluster);

typedef enum {
    ATTR_READ_ONLY = 0x01,
    ATTR_HIDDEN = 0x02,
    ATTR_SYSTEM = 0x04,
    ATTR_VOLUME_ID = 0x08,
    ATTR_DIRECTORY = 0x10,
    ATTR_ARCHIVE = 0x20,
    ATTR_LONG_NAME =    ATTR_READ_ONLY |
                        ATTR_HIDDEN |
                        ATTR_SYSTEM |
                        ATTR_VOLUME_ID,
} ATTR;

typedef struct {
    uint8_t     DIR_Name[12];
    uint8_t     DIR_Attr;
    uint32_t    DIR_FstClus;
    uint32_t    DIR_FileSize;
    wchar_t     LongName[256];
} DirEntry;

typedef struct {
    FAT         *fat;
    uint32_t    start_cluster;
    uint32_t    current_cluster;
    //TODO uint32_t    current_sector;
    uint32_t    current_offset;
} FatDir;

uint8_t FatDir_Init(FatDir *fatDir, FAT *fat, uint32_t start_cluster);

//returns 0 if entry was not read
uint8_t FatDir_GetNextEntry(FatDir *fatDir, DirEntry *dirEntry);

#endif /* FAT_H_ */
