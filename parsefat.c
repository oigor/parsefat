
#include <stdio.h>
#include <string.h>

#include "fat.h"
#include "disk_from_file.h"

//Дано
//
//
//Образ диска disk.img, отформатированного в FAT32.
//Имя образа передаётся программе через аргументы командной строки.
//Задача
//Используя только стандартные функции языка С (не используя никаких функций API
//операционной системы):
// Разобрать бинарный образ диска.
// Вывести в виде дерева список всех файлов и папок, содержащихся в этом образе.
//Пример
//C:\temp>parsefat myfat.img
//myfat.img
//│
//rootfile1.txt
//│
//rootfile2.txt
//│
//├───Directory_1
//│
//file1.txt
//│
//file2.txt
//│
//├───Directory_2
//│
//└───Directory_4
//│
//└───Directory_6
//│
//file1.txt
//│
//└───Directory_3
//│
//file.txt
//│
//└───Directory_5
//file.txt


void list_dir_recurcively(FatDir *dir, uint32_t current_depth) {
    //TODO simple draft
    DirEntry dirEntry;
    while(FatDir_GetNextEntry(dir, &dirEntry)) {
        if(dirEntry.DIR_Name[0] == '.' || (dirEntry.DIR_Attr & ATTR_VOLUME_ID) == ATTR_VOLUME_ID)
            continue;

        for(int i = 0; i < current_depth; i++) wprintf(L" | ");
        wprintf(L"%ls", dirEntry.LongName);
        if((dirEntry.DIR_Attr & ATTR_DIRECTORY) == ATTR_DIRECTORY) {
            FatDir fatDir;
            FatDir_Init(&fatDir, dir->fat, dirEntry.DIR_FstClus);
            wprintf(L" <DIR>\n");
            list_dir_recurcively(&fatDir, current_depth + 1);
        } else {
            wprintf(L"\n");
        }
    }
}

int main(int argc, char *argv[]) {
    Disk disk;
    FAT fat;
    FatDir fatDir;
    const char *filename = (argc > 1) ? argv[1] : "disk.img";
    DiskFromFile_Init(&disk, filename);
    FAT_Init(&fat, &disk);

    FatDir_Init(&fatDir, &fat, fat.bpb.BPB_RootClus);
    list_dir_recurcively(&fatDir, 0);

    return 0;
}
