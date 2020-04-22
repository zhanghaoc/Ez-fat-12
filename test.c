#include "definition.h"

int main() {
    char string[100] = "dossys.img";
    bool flag = OpenFAT12File("dossys.img");
    ReadFAT12LeadSecData();
    //伪造一个改名操作先-只需要更改pDirItem名字
    // DirItem* tempDirItem = (DirItem*)malloc(sizeof(DirItem));
    // ReadFAT12DirItemInfo(394+31, 3-1, tempDirItem);
    // char newname[] = "AHAJOIN";
    // memcpy(tempDirItem->szDirName, newname, 7);
    // //printf("%s\n", tempDirItem->szDirName);
    // WriteDirItemToImg("dossys.img", tempDirItem, 0x35240);


    //测试目录项
    // DirItem** pDirItem = (DirItem**)malloc(sizeof(DirItem*));

    // DWORD pdwSize;
    // if (ReadDirInfo(1, 394, pDirItem, &pdwSize)) 
    //     printf("ReadDirInfo success\n");
    // else
    //     printf("ReadDirInfo fail\n");
    // printf("pdwSize: %d\n", pdwSize);
    // for (int i = 0; i < pdwSize; i++) {
    //     printf("%d: %s | %d | %d | %lx\n", i, (*pDirItem)[i].szDirName, 
    // (*pDirItem)[i].dwDirAttr,(*pDirItem)[i].dwFirstClus,(*pDirItem)[i].dwFileSize);
    // }
    
    
}

