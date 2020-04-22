//coreFunc.c
#include "definition.h"

void DisplayDirItem();
void DisplayDir();
bool OpenFAT12File();
bool ReadFAT12LeadSecData();
bool ReadFAT12DirItemInfo();
bool ReadDirInfo();
DWORD FindFatClus(DWORD dwFatNum, char *lpFatAddr);
int FindFatClusList(DWORD dwFirstFatClus, char *lpFatAddr, DWORD *dwList);
bool ReadFat12Block();
bool SetDirItemClusList();
bool ReadClusList();
bool WriteDirItemToImg();
bool DeleteFile();
bool OnlyWriteDirToImg(const char *szFilePath, DirItem *pDirItem, int i, int j);

//全局变量
char floppyDrive[2880][512];
Fat12LeapSector MBR;

void DisplayDirItem(DirItem dirItem)
{
    printf("\nDirItemName:%s\nDirItemFirstClus:%d\n \
    DirItemFileSize:%ld\n",
           dirItem.szDirName,
           dirItem.dwFirstClus, dirItem.dwFileSize);
}
void displayDir(DirItem *dir, DWORD dwDirSize)
{
    DirItem *pDir = dir;
    for (int i = 0; i < dwDirSize; i++)
    {
        DisplayDirItem(*pDir);
        pDir++;
    }
}

//打开FAT12软盘
//建议整个读进一个数组，一块一个数组
//测试終了
bool OpenFAT12File(const char *szFilePath)
{
    //fopen
    FILE *hFile = fopen(szFilePath, "rb");
    if (hFile == NULL)
        return false;
    for (int i = 0; i < 2880; i++)
    {
        fread(floppyDrive[i], 512, 1, hFile);
    }
    fclose(hFile);
    return true;
    
}
//读取FAT12系统中首扇区512字节
//lpBuffer为缓冲区
bool ReadFAT12LeadSecData()
{
    memcpy(MBR.BS_JMPBOOT, floppyDrive[0], 3);
    memcpy(MBR.BS_OEMNAME, floppyDrive[0] + 3, 8);
    MBR.BS_OEMNAME[8] = 0;
    MBR.BPB_BYTS_PER_SEC = *(DWORD *)(floppyDrive[0] + 11);
    MBR.BPB_SEC_PER_CLUS = *(BYTE *)(floppyDrive[0] + 13);
    MBR.BPB_RSVD_SEC_CNT = *(DWORD *)&floppyDrive[0][14];
    MBR.BPB_NUM_FATS = *(BYTE *)&floppyDrive[0][16];
    MBR.BPB_ROOT_ENT_CNT = *(DWORD *)&floppyDrive[0][17];
    MBR.BPB_TOT_SEC16 = *(DWORD *)&floppyDrive[0][19];
    MBR.BPB_MEDIA = *(BYTE *)&floppyDrive[0][21];
    MBR.BPB_FAT_Sz16 = *(DWORD *)&floppyDrive[0][22];
    MBR.BPB_SEC_PER_TRK = *(DWORD *)&floppyDrive[0][24];
    MBR.BPB_NUM_HEADS = *(DWORD *)&floppyDrive[0][26];
    MBR.BPB_HIDD_SEC = *(LWORD *)&floppyDrive[0][28];
    MBR.BPB_TOT_SEC32 = *(LWORD *)&floppyDrive[0][32];
    MBR.BS_DRV_NUM = *(BYTE *)&floppyDrive[0][36];
    MBR.BS_RESERVED1 = *(BYTE *)&floppyDrive[0][37];
    MBR.BS_BOOT_SIG = *(BYTE *)&floppyDrive[0][38];
    MBR.BS_VOL_D = *(LWORD *)&floppyDrive[0][39];
    memcpy(MBR.BS_VOL_LAB, &floppyDrive[0][43], 11);
    MBR.BS_VOL_LAB[11] = 0;
    memcpy(MBR.BS_FILE_SYS_TYPE, &floppyDrive[0][54], 8);
    MBR.BS_FILE_SYS_TYPE[8] = 0;
    memcpy(MBR.LEAP_CODE, &floppyDrive[0][62], 448);
    MBR.END_MARK = *(DWORD *)&floppyDrive[0][510];

    printf("文件系统为FAT12\n");
    printf("OEM字符串\t%s\n", MBR.BS_OEMNAME);
    printf("每扇区字节数\t%d\n", MBR.BPB_BYTS_PER_SEC);
    printf("每簇占用扇区数\t%d\n", MBR.BPB_SEC_PER_CLUS);
    printf("boot占用扇区数\t%d\n", MBR.BPB_RSVD_SEC_CNT);
    printf("FAT表数目\t%d\n", MBR.BPB_NUM_FATS);
    printf("根目录文件最大值\t%d\n", MBR.BPB_ROOT_ENT_CNT);
    printf("扇区总数\t%d\n", MBR.BPB_TOT_SEC16);
    printf("每个FAT表占用扇区数\t%d\n", MBR.BPB_FAT_Sz16);
    printf("每磁道扇区数\t%d\n", MBR.BPB_SEC_PER_TRK);
    printf("磁头数\t%d\n", MBR.BPB_NUM_HEADS);
    printf("文件系统类型\t%s\n", MBR.BS_FILE_SYS_TYPE);

    return true;
}
//vi, touch后都需要递归更新时间，lastWriteTime
//当然，原版dos没有做，现在dos有做
//作用：将第sec扇区的父文件夹的名为lastDirStr的文件时间更改为originNameStr对应的4个字节
void changeTime(int sec, char* completeDir, char* originNameStr) {
    //由于根目录不存在父目录，这里直接return
    if (sec == 19)
        return;

    char* tempP = strrchr(completeDir, '/');
    *tempP = 0;
    tempP += 1;
    DirItem **pDirItem = (DirItem **)malloc(sizeof(DirItem *));
    DWORD pdwSize;
    ReadDirInfo(1, sec - 31, pDirItem, &pdwSize);
    int newSec = (*pDirItem)[1].dwFirstClus;
    newSec = (newSec == 0) ? 19 : newSec+31;
    DirItem **pDirItem1 = (DirItem **)malloc(sizeof(DirItem *));
    DWORD pdwSize1;
    if (newSec == 19)
    {
        ReadDirInfo(0, newSec, pDirItem1, &pdwSize1);
    }
    else
    {
        ReadDirInfo(1, newSec - 31, pDirItem1, &pdwSize1);
    }
 
    int j = 0;
    for (j = 0; j < pdwSize; j++) {
        if (!strcmp((*pDirItem)[j].szDirName, originNameStr))
            break;
    }
    int i = 0;
    for (i = 0; i < pdwSize1; i++) {
        if (!strcmp((*pDirItem1)[i].szDirName, tempP))
            break;
    }

    memcpy(floppyDrive[newSec]+32*(*pDirItem1)[i].row+22, floppyDrive[sec]+32*(*pDirItem)[j].row+22, 4);
    OnlyWriteDirToImg("dossys.img", (*pDirItem1)+32*(*pDirItem1)[i].row, newSec, i);

    changeTime(newSec, completeDir, (*pDirItem1)[i].szDirName);
}
//从第i扇区读出第j行目录项相关信息,ij均从0开始
bool ReadFAT12DirItemInfo(int i, int j, DirItem *pDirItem)
{
    char *lpBuffer = floppyDrive[i] + 32 * j;
    if (pDirItem == NULL)
        return false;
    BYTE tByte;
    CHAR szDirName[13] = {0};
    short tShort, tTime, tDate;
    int tInt;
    char tempc = 0xE5;
    memcpy(szDirName, lpBuffer, DIR_ITEM_NAME_LEN);
    if (szDirName[0] == tempc || *szDirName == 0)
        return false;
    //generate file name
    //若不是0结尾会undefined，这里考虑到属性位后面是0，肯定能结束
    //用文件属性判断好些
    if (lpBuffer[11] == 0x10)
    { //文件夹情况
        char *strp = strchr(lpBuffer, ' ');
        if (!strp)
        {
            memcpy(pDirItem->szDirName, szDirName, 8);
        }
        else
        {
            int pos = strp - lpBuffer;
            memcpy(pDirItem->szDirName, szDirName, pos);
        }
    }
    else
    {
        char *strp = strchr(lpBuffer, ' ');
        int pos;
        if (!strp)
        {
            pos = 8;
        }
        else
        {
            pos = strp - lpBuffer;
        }
        if (szDirName[8] == ' ')
        {
            memcpy(pDirItem->szDirName, szDirName, pos);
        }
        else
        {
            memcpy(pDirItem->szDirName, szDirName, pos);
            pDirItem->szDirName[pos] = '.';
            memcpy(pDirItem->szDirName + pos + 1, szDirName + 8, 3);
        }
    }

    //sprintf(pDirItem->szDirName, "%s.%s", lpBuffer, lpBuffer+8);
    //memcpy(pDirItem->szDirName,szDirName, DIR_ITEM_NAME_LEN+1);

    tTime = *(short *)(lpBuffer + DIR_ITEM_TIME_OFFSET);
    tDate = *(short *)(lpBuffer + DIR_ITEM_DATE_OFFSET);
    tByte = *(BYTE *)(lpBuffer + DIR_ITEM_ATTR_OFFSET);
    tShort = *(short *)(lpBuffer + DIR_ITEM_FIRST_CLUS_OFFSET);
    tInt = *(int *)(lpBuffer + DIR_ITEM_FILE_SIZE_OFFSET);
    pDirItem->dirWrtTime = tTime;
    pDirItem->dirWrtDate = tDate;
    pDirItem->dwDirAttr = tByte;
    pDirItem->dwFirstClus = tShort;
    pDirItem->dwFileSize = tInt;
    pDirItem->row = j;
    SetDirItemClusList(pDirItem, floppyDrive[1]);
    return true;
}
//读目录块信息，返回root或normal, 有效的DirItem的list,lpAddr即为list地址
//读取一个目录里所有元素，仅一个目录
//块数和簇号，统一选择用簇号
bool ReadDirInfo(int dwDirMode, int clusNumber, DirItem **lpAddr, DWORD *pdwSize)
{
    if (lpAddr == NULL)
        return false;
    DWORD dwDirItemNum = 0;
    int joffset;
    switch (dwDirMode)
    {
    case ROOT_DIRECTORY_MODE:
        dwDirItemNum = DIR_ITEM_MAX_NUM;
        joffset = 19;
        break;
    case NORMAL_DIRECTORY_MODE:
        dwDirItemNum = DIR_ITEM_NORMAL_NUM;
        joffset = 31 + clusNumber;
        break;
    default:
        //GetLastError();
        return false;
    }
    DWORD dwDirItemSize = sizeof(DirItem);
    DirItem *dirItemList = (DirItem *)malloc(dwDirItemNum * dwDirItemSize);
    //采用循环写法，当dwDirItemNum很大时也成立
    for (int i = 0; i < dwDirItemNum; i++)
        memset(&dirItemList[i], 0, dwDirItemSize);
    DWORD dwSize = 0;
    int flag = 0;
    for (int j = 0; j < dwDirItemNum / 16; j++) //每个块循环一次
    {
        //扫描每个目录项
        for (int i = 0; i < 16; i++)
        {
            if (ReadFAT12DirItemInfo(j + joffset, i, &dirItemList[dwSize]))
                dwSize++;
            //可能出现中间某个文件被删，为空，优化一下就是遇到0其实可以break
            // else {
            //     flag = 1;
            //     break;
            // }
        }
        // if (flag)
        //     break;
    }
    *pdwSize = dwSize;
    *lpAddr = (DirItem *)malloc(dwSize * dwDirItemSize);
    memset(*lpAddr, 0, dwSize * dwDirItemSize);
    memcpy(*lpAddr, dirItemList, dwSize * dwDirItemSize);
    free(dirItemList);
    return true;
}

//寻找簇号，lpFatAddr（FAT表起始）开始的，第dwFatNum个簇号
DWORD FindFatClus(DWORD dwFatNum, char *lpFatAddr)
{
    DWORD fatNext = 0xfff;
    DWORD dwItemBase;
    dwItemBase = dwFatNum / 2 * 3;
    if (dwFatNum % 2 == 1)
    {
        dwItemBase++;
        fatNext = *(short *)(lpFatAddr + dwItemBase);
        fatNext = fatNext >> 4; //34 12 = 12 34 _->123
    }
    else
    {
        fatNext = *(short *)(lpFatAddr + dwItemBase);
        fatNext = fatNext & 0xfff; //34 12 = 12 34 ->234
    }
    return fatNext;
}
//寻找某个文件的多个簇组成的数组//也直接在FAT表寻找
//不把fff记进来
int FindFatClusList(DWORD dwFirstFatClus, char *lpFatAddr, DWORD *dwList)
{
    DWORD dwFatClus = dwFirstFatClus;
    if (!dwFatClus)
    {
        *dwList = 0;
        return 0;
    }

    int i = 0;
    dwList[i] = dwFatClus;
    i++;
    dwFatClus = FindFatClus(dwFatClus, lpFatAddr);
    dwFatClus = dwFatClus & 0xfff;

    //printf("I have enter FinFatClusList\n");
    while (dwFatClus != 0xfff)
    {
        dwFatClus = FindFatClus(dwFatClus, lpFatAddr);
        dwFatClus = dwFatClus & 0xfff;
        dwList[i] = dwFatClus;
        i++;
        //printf("%.3X\n",dwFatClus);
    }
    return i;
}
//设置一个文件的簇
bool SetDirItemClusList(DirItem *pDirItem, char *lpFatSecBuffer)
{
    pDirItem->dwClusList = (DWORD *)malloc((pDirItem->dwFileSize / BYTES_PER_SECTION + 1) * sizeof(DWORD));
    memset(pDirItem->dwClusList, 0, (pDirItem->dwFileSize / BYTES_PER_SECTION + 1) * sizeof(DWORD));
    pDirItem->clusListNum = FindFatClusList(pDirItem->dwFirstClus, lpFatSecBuffer, pDirItem->dwClusList);
    return true;
}
//用于修改日期递归
bool OnlyWriteDirToImg(const char *szFilePath, DirItem *pDirItem, int i, int j) {
    FILE *hFile = fopen(szFilePath, "r+b");
    int offset = i * 0x200 + j * 32;
    //under "a" mode, fseek is ignored
    fseek(hFile, offset, SEEK_SET);
    int number = fwrite(floppyDrive[i] + j * 32, 32, 1, hFile);
    fclose(hFile);

    return true;
}
//把文件写回映像
//目录项、FAT项写回根目录区
//dwDirBaseAdd代表目录项是从头计算的第x个字节
//4个字节能够表示1.44MB = 1.44*2^20
//未修改floppyDrive
bool WriteDirItemToImg(const char *szFilePath, DirItem *pDirItem, int i, int j)
{
    FILE *hFile = fopen(szFilePath, "r+b");
    char *buffer;

    //写回目录项
    int offset = i * 0x200 + j * 32;
    //under "a" mode, fseek is ignored
    fseek(hFile, offset, SEEK_SET);

    // char dirBuffer[32] = {0};
    // memset(dirBuffer, ' ', 11);
    // memcpy(dirBuffer, pDirItem->szDirName, strlen(pDirItem->szDirName));
    // dirBuffer[11] = pDirItem->dwDirAttr;
    // memset(dirBuffer+12, 0, 10);
    // *(short*)(dirBuffer+22) = pDirItem->dirWrtTime;
    // *(short*)(dirBuffer+24) = pDirItem->dirWrtDate;
    // *(short*)(dirBuffer+26) = pDirItem->dwFirstClus;
    // *(long*)(dirBuffer+28) = pDirItem->dwFileSize;

    //printf("%s\n", dirBuffer);
    //&floppyDrive[i][j]与此写法有何不同？
    int number = fwrite(floppyDrive[i] + j * 32, 32, 1, hFile);

    //写回文件,在一个循环进行
    for (int i = 0; i < pDirItem->clusListNum; i++)
    {
        int clus = pDirItem->dwClusList[i];
        //写进数据区
        offset = (clus + 31) * 512;
        fseek(hFile, offset, SEEK_SET);
        fwrite(floppyDrive[clus + 31], 512, 1, hFile);
    }

    //每次只写回1个FAT项太麻烦了，于是每次都重新写18个扇区
    //FAT表起始地址是0x200, 即512
    fseek(hFile, 512, SEEK_SET);
    fwrite(floppyDrive[1], 512 * 18, 1, hFile);

    fclose(hFile);
    return true;
}

void SetFatClus(DWORD dwFatNum, char *lpFatAddr, int num)
{
    DWORD fatNext = 0xfff;
    DWORD dwItemBase;
    dwItemBase = dwFatNum / 2 * 3;
    if (dwFatNum % 2 == 1)
    {
        dwItemBase++;
        fatNext = *(short *)(lpFatAddr + dwItemBase);
        //34 12 = 12 34 -> 0004 = 04 00
        *(short *)(lpFatAddr + dwItemBase) = (fatNext & 0x000f) + (num << 4);
    }
    else
    {
        fatNext = *(short *)(lpFatAddr + dwItemBase);
        //34 12 = 12 34 -> 1000 = 00 10
        *(short *)(lpFatAddr + dwItemBase) = (fatNext & 0xf000) + num;
    }
}

bool DeleteFile(int sec, char *nameStr)
{
    DirItem **pDirItem = (DirItem **)malloc(sizeof(DirItem *));
    DWORD pdwSize;
    if (sec == 19)
    {
        ReadDirInfo(0, sec, pDirItem, &pdwSize);
    }
    else
    {
        ReadDirInfo(1, sec - 31, pDirItem, &pdwSize);
    }
    //直接略去.和..
    for (int i = 2; i < pdwSize; i++)
    {
        if (!strcmp(nameStr, (*pDirItem)[i].szDirName))
        {
            //修改文件名(floppy drive中的)
            *(BYTE *)(floppyDrive[sec] + i * 32) = 0xE5;
            //(*pDirItem)[i].szDirName[0] = 0xE5;
            //修改FAT表，将clusList里的FFF都设为0
            for (int j = 0; j < (*pDirItem)[i].clusListNum; j++)
                SetFatClus((*pDirItem)[i].dwClusList[j], floppyDrive[1], 0);
            //写回
            WriteDirItemToImg("dossys.img", 
            (*pDirItem) + (*pDirItem)[i].row * sizeof(pDirItem), sec, i);
            return true;
        }
    }
    return false;
}
void printnChar(char* str, int number) {
    for (int i = 0; i < number; i++)
        putchar(str[i]);
}
bool PrintFile(int sec, char *nameStr)
{
    DirItem **pDirItem = (DirItem **)malloc(sizeof(DirItem *));
    DWORD pdwSize;
    if (sec == 19)
    {
        ReadDirInfo(0, sec, pDirItem, &pdwSize);
    }
    else
    {
        ReadDirInfo(1, sec - 31, pDirItem, &pdwSize);
    }
    //直接略去.和..
    char* buff[513] = {0};
    for (int i = 2; i < pdwSize; i++)
    {
        if (!strcmp(nameStr, (*pDirItem)[i].szDirName))
        {
            if ((*pDirItem)[i].dwDirAttr != 0x20)
                continue;
            int filesize = (*pDirItem)[i].dwFileSize;
            for (int j = 0; j < (*pDirItem)[i].clusListNum; j++) {
                if (filesize > 512) {
                    filesize -= 512;
                    printnChar(floppyDrive[(*pDirItem)[i].dwClusList[j]+31], 512);
                } else {
                    printnChar(floppyDrive[(*pDirItem)[i].dwClusList[j]+31], filesize);
                }
            }
            return true;
        }
    }
    return false;
}
//ctrl+D
int getContent(char* buff) {
    printf("please input the content: press ctrl-b to end the file\n");
    int fileSize = 0;
    while((buff[fileSize] = fgetc(stdin)) != 2) {
        fileSize++;
    }
    //刷新feof();
    //clearerr(stdin);
    //去掉一个换行
    fgetc(stdin);
    return fileSize;
}
bool DeleteDirectory(int sec, char *nameStr)
{
    DirItem **pDirItem = (DirItem **)malloc(sizeof(DirItem *));
    DWORD pdwSize;
    if (sec == 19)
    {
        ReadDirInfo(0, sec, pDirItem, &pdwSize);
    }
    else
    {
        ReadDirInfo(1, sec - 31, pDirItem, &pdwSize);
    }

    int i;
    //直接略去.和..
    for (i = 0; i < pdwSize; i++)
    {
        if (!strcmp(nameStr, (*pDirItem)[i].szDirName))
            break;
    }
    //如果是文件
    if ((*pDirItem)[i].dwDirAttr == 0x20) {
        return DeleteFile(sec, nameStr);
    }
    //若是文件夹，开始递归
    DirItem **pDirItem1 = (DirItem **)malloc(sizeof(DirItem *));
    DWORD pdwSize1;
    int newSec = (*pDirItem)[i].dwFirstClus + 31;
    if (newSec == 19)
    {
        ReadDirInfo(0, newSec, pDirItem1, &pdwSize1);
    }
    else
    {
        ReadDirInfo(1, newSec - 31, pDirItem1, &pdwSize1);
    }
    //先将自身相关信息置0xE5
    //!!危险，i不能直接用于地址计算
    *(BYTE *)(floppyDrive[sec] + i * 32) = 0xE5;
    SetFatClus((*pDirItem)[i].dwFirstClus, floppyDrive[1], 0);
    //将子目录删掉
    for (int j = 0; j < pdwSize1; j++) {
        if (!strncmp((*pDirItem1)[j].szDirName, "..", 2)) {
            *(BYTE *)(floppyDrive[newSec] + j * 32) = 0xE5;
        } else if (!strncmp((*pDirItem1)[j].szDirName, ".", 1)) {
            *(BYTE *)(floppyDrive[newSec] + j * 32) = 0xE5;
        } else if ((*pDirItem1)[j].dwDirAttr == 0x10) {
            DeleteDirectory(newSec, (*pDirItem1)[j].szDirName);
        } else if ((*pDirItem1)[j].dwDirAttr == 0x20) {
            DeleteFile(newSec, (*pDirItem1)[j].szDirName);
        }
    }
    //pDir内存是malloc的，会一直在
    WriteDirItemToImg("dossys.img", (*pDirItem) + i * sizeof(pDirItem), sec, i);
    return true;
}
//0用于建立文件夹，1用于建立文件
bool CreateFile0(int curSec, char *nameStr)
{
    int newClus;

    for (int i = 2; i < 3072; i++)
    {
        if (!FindFatClus(i, floppyDrive[1]))
        {
            SetFatClus(i, floppyDrive[1], 0xfff);
            newClus = i;
            break;
        }
    }

    //得到现在时间
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    DWORD date = ((timeinfo->tm_year - 80) << 9) + ((timeinfo->tm_mon+1) << 5) +
                 timeinfo->tm_mday;
    DWORD time = (timeinfo->tm_hour << 11) + (timeinfo->tm_min << 5) +
                 timeinfo->tm_sec/2;

    char *DirStr = (char *)malloc(32);
    memset(DirStr, 0, 32);
    char *contentStr = (char *)malloc(64);
    ;
    memset(contentStr, 0, 64);

    memset(contentStr, ' ', 11);
    memcpy(contentStr, ".          ", 11);
    *(BYTE *)(contentStr + 11) = 0x10;
    *(short *)(contentStr + 26) = newClus;
    *(short *)(contentStr + 24) = date;
    *(short *)(contentStr + 22) = time;

    memset(contentStr + 32, ' ', 11);
    memcpy(contentStr + 32, "..         ", 11);
    *(BYTE *)(contentStr + 43) = 0x10;
    *(short *)(contentStr + 58) = (curSec == 19) ? 0 : (curSec - 31);
    *(short *)(contentStr + 56) = date;
    *(short *)(contentStr + 54) = time;

    memset(DirStr, ' ', 11);
    memcpy(DirStr, nameStr, strlen(nameStr));
    *(BYTE *)(DirStr + 11) = 0x10;
    *(short *)(DirStr + 26) = newClus;
    *(short *)(DirStr + 24) = date;
    *(short *)(DirStr + 22) = time;

    return CreateFile(curSec, DirStr, contentStr, 64);
}
int getEmptyClus() {
    int newClus;

    for (int i = 2; i < 3072; i++)
    {
        if (!FindFatClus(i, floppyDrive[1]))
        {
            SetFatClus(i, floppyDrive[1], 0xfff);
            newClus = i;
            break;
        }
    }
    return newClus;
}
bool CopyFile(int sec1, int sec2, char* firstName, char* secondName) {
    DirItem **pDirItem1 = (DirItem **)malloc(sizeof(DirItem *));
    DWORD pdwSize1;
    if (sec1 == 19)
    {
        ReadDirInfo(0, sec1, pDirItem1, &pdwSize1);
    }
    else
    {
        ReadDirInfo(1, sec1 - 31, pDirItem1, &pdwSize1);
    }
    int i = 0;
    for (i = 0; i < pdwSize1; i++) {
        if (!strcmp(firstName, (*pDirItem1)[i].szDirName)) {
            break;
        }    
    }
    if (i == pdwSize1)
        return false;
    int size1 = (*pDirItem1)[i].dwFileSize;
    DirItem **pDirItem2 = (DirItem **)malloc(sizeof(DirItem *));
    DWORD pdwSize2;
    if (sec2 == 19)
    {
        ReadDirInfo(0, sec2, pDirItem2, &pdwSize2);
    }
    else
    {
        ReadDirInfo(1, sec2 - 31, pDirItem2, &pdwSize2);
    }
    int j = 0;
    for (j = 0; j < pdwSize2; j++) {
        if (!strcmp(secondName, (*pDirItem2)[j].szDirName))
            break;
    }
    if (j == pdwSize2)
        return false;
    int size2 = (*pDirItem2)[j].dwFileSize;
    int totalSize = size1 + size2;

    char* buff = (char*)malloc(1024);
    memcpy(buff, floppyDrive[(*pDirItem1)[i].dwFirstClus+31], size1);
    memcpy(buff+size1, floppyDrive[(*pDirItem2)[j].dwFirstClus+31], size2);
    DeleteFile(sec1, firstName);
    CreateFile1(sec1, firstName, buff, totalSize);
    free(buff);
    return true;
}
bool CreateFile1(int curSec, char *nameStr, char* content, int contentLen)
{
    int newClus = getEmptyClus();

    //得到现在时间
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    DWORD date = ((timeinfo->tm_year - 80) << 9) + ((timeinfo->tm_mon+1) << 5) +
                 timeinfo->tm_mday;
    DWORD time = (timeinfo->tm_hour << 11) + (timeinfo->tm_min << 5) +
                 timeinfo->tm_sec;

    char *DirStr = (char *)malloc(32);
    memset(DirStr, 0, 32);
    memset(DirStr, ' ', 11);
    *(BYTE *)(DirStr + 11) = 0x20;
    *(short *)(DirStr + 26) = newClus;
    *(short *)(DirStr + 24) = date;
    *(short *)(DirStr + 22) = time;
    *(int *)(DirStr + 28) = contentLen;

    char *temp = strchr(nameStr, '.');
    if (!temp)
    {
        memcpy(DirStr, nameStr, strlen(nameStr));
    }
    else
    {
        memcpy(DirStr, nameStr, temp - nameStr);
        memcpy(DirStr + 8, temp + 1, strlen(temp + 1));
    }
    // printf("Create1--%s\n", DirStr);

    CreateFile(curSec, DirStr, content, contentLen);
    free(DirStr);
    return true;
}

bool CreateFile(int sec, char *DirStr, char *content, int contentlen)
{
    DirItem **pDirItem = (DirItem **)malloc(sizeof(DirItem *));
    DWORD pdwSize;

    if (sec == 19)
    {
        ReadDirInfo(0, sec, pDirItem, &pdwSize);

        if (pdwSize >= 224)
        {
            printf("root directory is full\n");
            return false;
        }
    }
    else
    {
        ReadDirInfo(1, sec - 31, pDirItem, &pdwSize);
        //暂时不考虑无限子目录的情况，头秃
    }
    char tempc = 0xE5;
    char *tempP = floppyDrive[sec];
    int i;
    for (i = 0; i < 16; i++) {
        if ((tempP+32*i)[0] == tempc || *(tempP+32*i) == 0) {
            memcpy(tempP+32*i, DirStr, 32);
            break;
        }
    }

    DirItem temp;
    ReadFAT12DirItemInfo(sec, i, &temp);

    //
    int newClus;
    int currentClus = temp.dwFirstClus;
    DWORD ClusBuff[10];
    int ClusNumber = 0;
    ClusBuff[ClusNumber++] = currentClus;
    while(contentlen > 512) {
        newClus = getEmptyClus();
        SetFatClus(currentClus, floppyDrive[1], newClus);
        memset(floppyDrive[currentClus + 31], 0, 512);
        memcpy(floppyDrive[currentClus + 31], content, 512);
        temp.clusListNum++;
        currentClus = newClus;
        ClusBuff[ClusNumber++] = currentClus;
        contentlen -= 512;
    }
    //写文件时不用清空，但建立子目录时必须清空
    memset(floppyDrive[currentClus + 31], 0, 512);
    memcpy(floppyDrive[currentClus + 31], content, contentlen);

    free(temp.dwClusList);
    temp.dwClusList = (DWORD*)malloc(sizeof(DWORD)*ClusNumber);
    memcpy(temp.dwClusList, ClusBuff, sizeof(DWORD)*ClusNumber);
    //i表示此扇区第i个目录项，这里不能是pdwSize
    WriteDirItemToImg("dossys.img", &temp, sec, i);

    return true;
}
bool checkName(int sec, char *nameStr)
{
    DirItem **pDirItem = (DirItem **)malloc(sizeof(DirItem *));
    DWORD pdwSize;
    if (sec == 19)
    {
        ReadDirInfo(0, sec, pDirItem, &pdwSize);
    }
    else
    {
        ReadDirInfo(1, sec - 31, pDirItem, &pdwSize);
    }
    for (int i = 0; i < pdwSize; i++)
    {
        if (!strcmp(nameStr, (*pDirItem)[i].szDirName))
        {
            return false;
        }
    }
    return true;
}
//从盘读簇
bool ReadClusList(DWORD *Dirlist, char *lpBuffer) {}