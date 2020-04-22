#ifndef DEFINITION_H
#define DEFINITION_H

#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


#define BYTES_PER_SECTOR 512
#define DIR_ITEM_NAME_LEN 11
#define DIR_ITEM_ATTR_OFFSET 11
#define DIR_ITEM_TIME_OFFSET 22
#define DIR_ITEM_DATE_OFFSET 24
#define DIR_ITEM_FIRST_CLUS_OFFSET 26
#define DIR_ITEM_FILE_SIZE_OFFSET 28
#define ROOT_DIRECTORY_MODE 0 //root-0
#define NORMAL_DIRECTORY_MODE 1
#define DIR_ITEM_MAX_NUM 224
#define DIR_ITEM_NORMAL_NUM 16 //可以无限个
#define BYTES_PER_SECTION 0x200


typedef char CHAR;
typedef char BYTE;
typedef unsigned short DWORD;  //短整数，一个字，双字节
typedef long LWORD;  //长整数，双字，四字节
typedef struct Fat12LeapSector
{
    CHAR    BS_JMPBOOT[3];
    CHAR    BS_OEMNAME[9];
    DWORD   BPB_BYTS_PER_SEC; //每个扇区字节数0x200
    BYTE    BPB_SEC_PER_CLUS; //每簇扇区数
    DWORD   BPB_RSVD_SEC_CNT; //boot记录占用扇区
    BYTE    BPB_NUM_FATS;     //FAT表数目
    DWORD   BPB_ROOT_ENT_CNT; //根目录文件最大值0xE0(224)
    DWORD   BPB_TOT_SEC16;    //扇区总数2880
    BYTE    BPB_MEDIA;        
    DWORD   BPB_FAT_Sz16;     //每FAT扇区数
    DWORD   BPB_SEC_PER_TRK;  //每磁道扇区数
    DWORD   BPB_NUM_HEADS;    //磁头数
    LWORD   BPB_HIDD_SEC;     
    LWORD   BPB_TOT_SEC32;
    BYTE    BS_DRV_NUM;
    BYTE    BS_RESERVED1;
    BYTE    BS_BOOT_SIG;
    LWORD   BS_VOL_D;
    CHAR    BS_VOL_LAB[12];
    CHAR    BS_FILE_SYS_TYPE[9];
    CHAR    LEAP_CODE[448];
    DWORD   END_MARK;
} Fat12LeapSector;

typedef struct DirItem //目录项
{
    CHAR    szDirName[13]; //需要留1个0,还有一个.
    BYTE    dwDirAttr; //卷标项是0x28，文件项是0x20，目录是0x10
    CHAR    reversed[10];
    DWORD   dirWrtTime;
    DWORD   dirWrtDate;
    DWORD   dwFirstClus;
    LWORD   dwFileSize;
    DWORD*  dwClusList;
    int     clusListNum;
    int     row;//i并不是随时等于物理上的行数，所以这里需要此变量
} DirItem;

void DisplayDirItem();
void DisplayDir();
bool OpenFAT12File();
bool ReadFAT12LeadSecData();
bool ReadFAT12DirItemInfo();
bool ReadDirInfo();
DWORD FindFatClus(DWORD dwFatNum, char* lpFatAddr);
int FindFatClusList(DWORD dwFirstFatClus, char* lpFatAddr, DWORD *dwList);
bool ReadFat12Block();
bool SetDirItemClusList();
bool ReadClusList();
bool WriteDirItemToImg();
bool DeleteFile();
bool checkName();
bool CreateFile();
bool CreateFile0();
bool CreateFile1();
bool DeleteDirectory();
bool PrintFile();
bool OnlyWriteDirToImg();
void changeTime();
int getContent();
bool CopyFile();

#endif