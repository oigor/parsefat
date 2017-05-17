/*
 * fat.c
 *
 *  Created on: May 14, 2017
 *      Author: igor
 */

#include <string.h>

#include "fat.h"

static inline uint16_t le_buf_to_16(const uint8_t *buf) {
    return (uint16_t)buf[0] | (((uint16_t)buf[1]) << 8);
}

static inline uint32_t le_buf_to_32(const uint8_t *buf) {
    return (uint32_t)buf[0] | (((uint32_t)buf[1]) << 8)| (((uint32_t)buf[2]) << 16)| (((uint32_t)buf[3]) << 24);
}

//Count of bytes per sector. This value may take on
//only the following values: 512, 1024, 2048 or 4096.
static inline uint16_t parse_BPB_BytsPerSec(const uint8_t *bpb_buf) {
    return le_buf_to_16(bpb_buf + 11);
}

//Number of sectors per allocation unit. This value
//must be a power of 2 that is greater than 0. The
//legal values are 1, 2, 4, 8, 16, 32, 64, and 128.
static inline uint8_t parse_BPB_SecPerClus(const uint8_t *bpb_buf) {
    return bpb_buf[13];
}

//Number of reserved sectors in the reserved region
//of the volume starting at the first sector of the
//volume. This field is used to align the start of the
//data area to integral multiples of the cluster size
//with respect to the start of the partition/media.
//This field must not be 0 and can be any non-zero
//value.
//This field should typically be used to align the start
//of the data area (cluster #2) to the desired
//alignment unit, typically cluster size.
static inline uint16_t parse_BPB_RsvdSecCnt(const uint8_t *bpb_buf) {
    return le_buf_to_16(bpb_buf + 14);
}

//The count of file allocation tables (FATs) on the
//volume. A value of 2 is recommended although a
//value of 1 is acceptable.
static inline uint8_t parse_BPB_NumFATs(const uint8_t *bpb_buf) {
    return bpb_buf[16];
}

//This field is the new 32-bit total count of sectors on
//the volume. This count includes the count of all
//sectors in all four regions of the volume.
//This field can be 0; if it is 0, then BPB_TotSec16
//must be non-zero. For FAT12/FAT16 volumes, this
//field contains the sector count if BPB_TotSec16 is
//0 (count is greater than or equal to 0x10000).
//For FAT32 volumes, this field must be non-zero.
static inline uint32_t parse_BPB_TotSec32(const uint8_t *bpb_buf) {
    return le_buf_to_32(bpb_buf+ 32);
}

//This field is the FAT32 32-bit count of sectors occupied
//by one FAT.
//Note that BPB_FATSz16 must be 0 for media formatted
//FAT32.
static inline uint32_t parse_BPB_FATSz32(const uint8_t *bpb_buf) {
    return le_buf_to_32(bpb_buf+ 36);
}

//This is set to the cluster number of the first cluster of
//the root directory,
//This value should be 2 or the first usable (not bad)
//cluster available thereafter.
static inline uint32_t parse_BPB_RootClus(const uint8_t *bpb_buf) {
    return le_buf_to_32(bpb_buf+ 44);
}

static uint8_t parse_bpb(BPB *bpb, const uint8_t *bpb_buf) {
    //parses fat32 only
    bpb->BPB_BytsPerSec = parse_BPB_BytsPerSec(bpb_buf);
    bpb->BPB_SecPerClus = parse_BPB_SecPerClus(bpb_buf);
    bpb->BPB_RsvdSecCnt = parse_BPB_RsvdSecCnt(bpb_buf);
    bpb->BPB_NumFATs    = parse_BPB_NumFATs(bpb_buf);
    bpb->BPB_TotSec32   = parse_BPB_TotSec32(bpb_buf);
    bpb->BPB_FATSz32    = parse_BPB_FATSz32(bpb_buf);
    bpb->BPB_RootClus   = parse_BPB_RootClus(bpb_buf);
    return 0;
}

uint8_t FAT_Init(FAT *fat, Disk *disk) {
    uint8_t bpb_buf[512];
    fat->disk = disk;
    Disk_Read(fat->disk, 0, bpb_buf, 512);
    parse_bpb(&fat->bpb, bpb_buf);
    return 0;
}

uint32_t FAT_GetNextCluster(FAT *fat, uint32_t current_cluster) {
    uint8_t buf[4];
    uint32_t FATOffset = current_cluster * 4;
    uint32_t ThisFATSecNum = fat->bpb.BPB_RsvdSecCnt + (FATOffset / fat->bpb.BPB_BytsPerSec);
    uint32_t ThisFATEntOffset = FATOffset % fat->bpb.BPB_BytsPerSec;
    uint32_t SectorNumber = (fat->bpb.BPB_NumFATs * fat->bpb.BPB_FATSz32) + ThisFATSecNum;
    Disk_Read(fat->disk, SectorNumber * fat->bpb.BPB_BytsPerSec + ThisFATEntOffset, buf, 4);
    //Disk_Read(fat->disk, fat->bpb.BPB_RsvdSecCnt + current_cluster * 4, buf, 4);
    //FAT32ClusEntryVal = (*((DWORD *) &SecBuff[ThisFATEntOffset])) & 0x0FFFFFFF;
    return le_buf_to_32(buf) & 0x0FFFFFFF;
}

uint8_t FatDir_Init(FatDir *fatDir, FAT *fat, uint32_t start_cluster) {
    fatDir->fat = fat;
    fatDir->start_cluster = start_cluster;
    fatDir->current_cluster = fatDir->start_cluster;
    fatDir->current_offset = 0;
    return 0;
}

static uint32_t FatDir_ClusterToAddress(FAT *fat, uint32_t cluster) {
    return ((cluster - 2) * fat->bpb.BPB_SecPerClus + fat->bpb.BPB_RsvdSecCnt + fat->bpb.BPB_NumFATs * fat->bpb.BPB_FATSz32) * fat->bpb.BPB_BytsPerSec;
}

uint8_t parse_long_dir_entry(DirEntry *dir_entry, const uint8_t *dir_entry_buf) {
    uint8_t LDIR_Ord = (dir_entry_buf[0] & (~0x40)) - 1; //LDIR_Ord is 1-based, but 0-based is more convenient

    for(int i = 0; i < 5; i++) {
        dir_entry->LongName[LDIR_Ord * 13 + i] = le_buf_to_16(dir_entry_buf + 1 + i * 2);
    }

    for(int i = 5; i < 11; i++) {
        dir_entry->LongName[LDIR_Ord * 13 + i] = le_buf_to_16(dir_entry_buf + 14 + (i - 5) * 2);
    }

    for(int i = 11; i < 13; i++) {
        dir_entry->LongName[LDIR_Ord * 13 + i] = le_buf_to_16(dir_entry_buf + 28 + (i - 11) * 2);
    }

    if((dir_entry_buf[0] & 0x40) == 0x40) {
        dir_entry->LongName[LDIR_Ord * 13 + 13] = '\0';
    }
    return 0;
}

uint8_t parse_dir_entry(DirEntry *dir_entry, const uint8_t *dir_entry_buf) {
    memcpy(dir_entry->DIR_Name, dir_entry_buf, 11);
    //TODO process spaces in DIR_Name
    dir_entry->DIR_Name[11] = '\0';
    dir_entry->DIR_Attr = dir_entry_buf[11];
    dir_entry->DIR_FstClus = ((uint32_t)le_buf_to_16(dir_entry_buf + 26)) | ((uint32_t)le_buf_to_16(dir_entry_buf + 20) << 16);
    dir_entry->DIR_FileSize = le_buf_to_32(dir_entry_buf + 28);
    return 0;
}

static uint8_t check_move_cluster(FatDir *fatDir, uint32_t *disk_offset) {
    if(fatDir->current_offset == fatDir->fat->bpb.BPB_SecPerClus * fatDir->fat->bpb.BPB_BytsPerSec) {
        uint32_t nextCluster = FAT_GetNextCluster(fatDir->fat, fatDir->current_cluster);
        if(nextCluster == CLUSTER_FINAL)
            return 0;
        else {
            fatDir->current_cluster = nextCluster;
            fatDir->current_offset = 0;
            *disk_offset = FatDir_ClusterToAddress(fatDir->fat, fatDir->current_cluster);
        }
    }
    return 1;
}

uint8_t FatDir_GetNextEntry(FatDir *fatDir, DirEntry *dirEntry) {
    //TODO this can be optimized by reading sector by sector into buffer
    //TODO check cluster boundaries
    uint8_t buf[32];
    uint32_t disk_offset = FatDir_ClusterToAddress(fatDir->fat, fatDir->current_cluster);
    Disk_Read(fatDir->fat->disk, disk_offset + fatDir->current_offset, buf, 32);
    fatDir->current_offset += 32;

//    if(!check_move_cluster(fatDir, &disk_offset))
//        return 0;

    if((buf[11] & ATTR_LONG_NAME) == ATTR_LONG_NAME) {
        uint8_t LDIR_Ord = (buf[0] & (~0x40));
        for(int i = LDIR_Ord; i >= 1; i--) {
            parse_long_dir_entry(dirEntry, buf);
            Disk_Read(fatDir->fat->disk, disk_offset + fatDir->current_offset, buf, 32);
            fatDir->current_offset += 32;

//            if(!check_move_cluster(fatDir, &disk_offset))
//                return 0;
        }
    }
    parse_dir_entry(dirEntry, buf);

    return (dirEntry->DIR_Name[0] != 0xE5 && dirEntry->DIR_Name[0] != 0x00);
}

