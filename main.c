#include "definition.h"

#define DISK_NAME_MAX 100

//全局变量
//当前目录对应扇区,不是簇号
int currentSec;
//路径格式为/xxxx/xxx
char currentDir[100] = {'/'};
char commandBuf[100] = {0};
//未测试，多页文件
bool initialize() {
    printf("检查首扇区信息...");
    if(ReadFAT12LeadSecData()) {
        printf("验证完成！\n");
        return true;
    }
    else {
        printf("验证失败！\n");
        return false;
    }   
}

void printName() {
    printf("\033[0;32m");
    printf("FAT12@zhx");
    printf("\033[0m");
    printf(":");
    //print current directory
    printf("\033[0;34m");
    printf("%s", currentDir);
    printf("\033[0m");
    printf("$ ");
}
void ls() {
    DirItem** pDirItem = (DirItem**)malloc(sizeof(DirItem*));
    DWORD pdwSize;
    if (currentSec == 19) {
        ReadDirInfo(0, currentSec, pDirItem, &pdwSize);
    } else {
        ReadDirInfo(1, currentSec-31, pDirItem, &pdwSize);
    }
    //日期-时间-<DIR>-大小-文件名
    for (int i = 0; i < pdwSize; i++) {
        int year = ((*pDirItem)[i].dirWrtDate >> 9) + 1980;
        int month = ((*pDirItem)[i].dirWrtDate >> 5) & 15;
        int day = (*pDirItem)[i].dirWrtDate & 31;
        int hour = ((*pDirItem)[i].dirWrtTime >> 11) & 31;
        int minute = ((*pDirItem)[i].dirWrtTime >> 5) & 63;
        printf("%04d/%02d/%02d\t%02d:%02d\t", year, month, day, hour, minute);
        if ((*pDirItem)[i].dwDirAttr == 0x10) {
            char temp[] = "<DIR>";
            printf("%s\t\t%s\n", temp, (*pDirItem)[i].szDirName);
        }
        else {
            printf("\t%ld\t%s\n", (*pDirItem)[i].dwFileSize, (*pDirItem)[i].szDirName);
        }
    }
    //有空再做内存管理
    free(pDirItem);
}
//flag determine if show '|'
void printTree(int sec, int level, int flag[]) {
    flag[level] = 1;
    DirItem** pDirItem = (DirItem**)malloc(sizeof(DirItem*));
    DWORD pdwSize;
    if (sec == 19) {
        ReadDirInfo(0, sec, pDirItem, &pdwSize);
    } else {
        ReadDirInfo(1, sec-31, pDirItem, &pdwSize);
    }
    char tempc = 0xE5;
    for (int i = 0; i < pdwSize; i++) {
        if (i == pdwSize-1) flag[level] = 0;
        if ((*pDirItem)[i].dwDirAttr == 0x10) {
            if (strncmp((*pDirItem)[i].szDirName, ".", 1) && strncmp((*pDirItem)[i].szDirName, "..", 2)) {
                for (int j = 0; j < level; j++) {
                    if (flag[j]) printf("|   ");
                    else printf("    ");
                }
                printf("|---%s\n", (*pDirItem)[i].szDirName);
                printTree((*pDirItem)[i].dwFirstClus+31, level+1, flag);
            }
        } else {
            for (int j = 0; j < level; j++) {
                if (flag[j]) printf("|   ");
                else printf("    ");
            }
            printf("|---%s\n", (*pDirItem)[i].szDirName);
        }
    }
}
void tree() {
    //暂时上限设为10层
    int flag[10] = {0};
    printTree(currentSec, 0, flag);
}
//currentDir和lastDir都是扇区号
//输入保证了全都是文件夹//cd的除外
int checkDir(char* name, int currentDir, int lastDir) {
    //空字符串情况
    if (!*name)
        return currentDir;
    char* pstr = strchr(name, '/');
    int len;
    //只剩一个文件的情况
    if (!pstr) {
        //由于此函数只对文件夹进行检查，返回true
        //用'.'判断文件和文件夹其实不好，以后可优化
        if(strchr(name, '.'))
            return lastDir;
        else {
            pstr = name + strlen(name);
            len = strlen(name);
        }
    } else {
        len = pstr-name;
        pstr = pstr + 1;
    }
    
    DirItem** pDirItem = (DirItem**)malloc(sizeof(DirItem*));
    DWORD pdwSize;
    if (currentDir == 19) {
        ReadDirInfo(0, currentDir, pDirItem, &pdwSize);
    } else {
        ReadDirInfo(1, currentDir-31, pDirItem, &pdwSize);
    }
    for (int i = 0; i < pdwSize; i++) {
        if (!strncmp(name, (*pDirItem)[i].szDirName, len) 
        && len == strlen((*pDirItem)[i].szDirName) 
        && (*pDirItem)[i].dwDirAttr == 0x10) {
            return checkDir(pstr, (*pDirItem)[i].dwFirstClus+31, currentDir);
        }
            
    }
    return 0;
}
void cd() {
    if (commandBuf[3] == '.') {
        //cd ..的情况
        if (commandBuf[4] == '.' && strlen(commandBuf) == 5) {
            char* temp = strrchr(currentDir, '/');
            if (temp == currentDir) {
                if (temp[1])
                    temp[1] = 0;
                //说明已经在根目录
                else 
                    return;
            } else {
                *temp = 0;
            }
            DirItem** pDirItem = (DirItem**)malloc(sizeof(DirItem*));
            DWORD pdwSize;
            //此时肯定是在非根目录去区
            ReadDirInfo(1, currentSec-31, pDirItem, &pdwSize);
            int clus = (*pDirItem)[1].dwFirstClus;
            //clus为0说明是根目录
            if (clus)
                currentSec = clus + 31;
            else 
                currentSec = 19;
            return;
        }
        //cd . 的情况
        if (strlen(commandBuf) == 4)
            return;
    }
    //cd ''空的情况
    if (strlen(commandBuf) < 4) {
        printf("No such file or directory\n");
        return;
    }
    //其他情况
    int Sec;
    if (Sec = checkDir(commandBuf+3, currentSec, 0)) {
        //根目录只有/
        if (currentDir[1])
            strcat(currentDir, "/");
        strcat(currentDir, commandBuf+3);
        currentSec = Sec;
        //牢记，所有函数输入输出都是扇区号
    } else {
        printf("No such file or directory\n");
    }
}
char* getLastName(char* nameStr) {
    char* temp = strrchr(nameStr,'/');
    if (!temp)
        return nameStr;
    //不应以.分辨文件，这里直接不用判断
    //if (strchr(temp, '.'))
        return temp+1;
}
//删文件，不是文件夹
void rm() {
    if (commandBuf[3] == '.') {
        //cd ..的情况
        if (commandBuf[4] == '.' && strlen(commandBuf) == 5) {
            printf("you can't delete ..\n");
            return;
        }
        //cd . 的情况
        if (strlen(commandBuf) == 4) {
            printf("you can't delete .\n");
            return;
        }            
    }
    //cd ''空的情况
    if (strlen(commandBuf) < 4) {
        printf("No such file or directory\n");
        return;
    }
    //其他情况
    char* nameStr = getLastName(commandBuf+3);
    nameStr[-1] = 0;
    int sec;

    if (commandBuf+3 != nameStr) {
        if (sec = checkDir(commandBuf+3, currentSec, 0)) {
            ;
        } else {
            printf("No such file or directory\n");
            return;
        }
    } else {
        sec = currentSec;
    }
    if (DeleteFile(sec, nameStr)) 
        ;
    else
        printf("No such file or directory\n");
}
void type() {
    if (commandBuf[5] == '.') {
        //cd ..的情况
        if (commandBuf[6] == '.' && strlen(commandBuf) == 7) {
            printf("you can't delete ..\n");
            return;
        }
        //cd . 的情况
        if (strlen(commandBuf) == 6) {
            printf("you can't delete .\n");
            return;
        }            
    }
    //cd ''空的情况
    if (strlen(commandBuf) < 6) {
        printf("No such file or directory\n");
        return;
    }
    //其他情况
    char* nameStr = getLastName(commandBuf+5);
    nameStr[-1] = 0;
    int sec;

    if (commandBuf+5 != nameStr) {
        if (sec = checkDir(commandBuf+5, currentSec, 0)) {
            ;
        } else {
            printf("No such file or directory\n");
            return;
        }
    } else {
        sec = currentSec;
    }
    if (PrintFile(sec, nameStr)) 
        ;
    else
        printf("No such file or directory\n");
}
void rd() {
    if (commandBuf[3] == '.') {
        //cd ..的情况
        if (commandBuf[4] == '.' && strlen(commandBuf) == 5) {
            printf("you can't delete ..\n");
            return;
        }
        //cd . 的情况
        if (strlen(commandBuf) == 4) {
            printf("you can't delete .\n");
            return;
        }            
    }
    //cd ''空的情况
    if (strlen(commandBuf) < 4) {
        printf("No such file or directory\n");
        return;
    }
    //其他情况
    char* nameStr = getLastName(commandBuf+3);
    nameStr[-1] = 0;
    int sec;

    if (commandBuf+3 != nameStr) {
        if (sec = checkDir(commandBuf+3, currentSec, 0)) {
            ;
        } else {
            printf("No such file or directory\n");
            return;
        }
    } else {
        sec = currentSec;
    }
    if (DeleteDirectory(sec, nameStr)) 
        ;
    else
        printf("No such file or directory\n");
}
void mkdir() {
    if (commandBuf[6] == '.') {
        //cd ..的情况
        if (commandBuf[7] == '.' && strlen(commandBuf) == 5) {
            printf("you can't create ..\n");
            return;
        }
        //cd . 的情况
        if (strlen(commandBuf) == 7) {
            printf("you can't create .\n");
            return;
        }            
    }
    //cd ''空的情况
    if (strlen(commandBuf) < 7) {
        printf("please enter a valid name\n");
        return;
    }
    char* nameStr = getLastName(commandBuf+6);
    nameStr[-1] = 0;
    //其他情况
    int sec;
    //确认前面路径正确
    if (commandBuf+6 != nameStr) {
        if (sec = checkDir(commandBuf+6, currentSec, 0)) {

        } else {
            printf("No such file or directory\n");
            return;
        }
    } else {
        sec = currentSec;
    }
    //检查是否重名
    if (!checkName(sec, nameStr)) {
        printf("Directory already exists\n");
        return;
    }
    //CreateFile1用于建立文件
    if (CreateFile0(sec, nameStr))  {
        return;
    } 
    printf("No such file or directory\n");
}

int checkDirTure(char* name) {
    if (name[0] == '.') {
        //cd ..的情况
        if (name[1] == '.' && strlen(name) == 2) {
            printf("you can't edit ..\n");
            return 0;
        }
        //cd . 的情况
        if (strlen(name) == 1) {
            printf("you can't edit .\n");
            return 0;
        }            
    }
    //cd ''空的情况
    if (strlen(name) == 1) {
        printf("No such file or directory\n");
        return 0;
    }
    char* nameStr = getLastName(name);
    nameStr[-1] = 0;
    int sec;
    int flag = 0;
    if (name != nameStr) {
        flag = 1;
        if (sec = checkDir(name, currentSec, 0)) {
            ;
        } else {
            printf("No such file or directory\n");
            return 0;
        }
    } else {
        sec = currentSec;
    }
    if (flag)
        nameStr[-1] = '/';
    return sec;
}

void copy() {
    //未进行去空格控制
    char* secondName = strrchr(commandBuf, ' ');
    *secondName = 0;
    secondName += 1;
    char* firstName = strchr(commandBuf, ' ');
    *firstName = 0;
    firstName += 1;
    
    int sec1 = checkDirTure(firstName);
    int sec2 = checkDirTure(secondName);

    if (!sec1 || !sec2) {
        printf("No such file or directory\n");
        return;
    }

    firstName = getLastName(firstName);
    secondName = getLastName(secondName);
    
    char* completeDir = (char*)malloc(64);
    memset(completeDir, 0, 64);
    memcpy(completeDir, currentDir, strlen(currentDir));
    strcat(completeDir, "/");
    strcat(completeDir, commandBuf+6);//+4

    if (CopyFile(sec1, sec2, firstName, secondName)) 
        ;
    else
        printf("No such file or directory\n");
}
 
void edit() {
    if (commandBuf[5] == '.') {
        //cd ..的情况
        if (commandBuf[6] == '.' && strlen(commandBuf) == 7) {
            printf("you can't edit ..\n");
            return;
        }
        //cd . 的情况
        if (strlen(commandBuf) == 6) {
            printf("you can't edit .\n");
            return;
        }            
    }
    //cd ''空的情况
    if (strlen(commandBuf) < 6) {
        printf("No such file or directory\n");
        return;
    }

    char* completeDir = (char*)malloc(64);
    memset(completeDir, 0, 64);
    memcpy(completeDir, currentDir, strlen(currentDir));
    strcat(completeDir, "/");
    strcat(completeDir, commandBuf+6);

    //其他情况
    char* nameStr = getLastName(commandBuf+5);
    nameStr[-1] = 0;
    int sec;

    if (commandBuf+5 != nameStr) {
        if (sec = checkDir(commandBuf+5, currentSec, 0)) {
            ;
        } else {
            printf("No such file or directory\n");
            return;
        }
    } else {
        sec = currentSec;
    }
    //先删掉再create
    
    if (DeleteFile(sec, nameStr)) {
        ;
    } else {
        printf("No such file or directory\n");
        return;
    }
    char* buff = (char*)malloc(1024);
    int fileSize = getContent(buff);
    printf("edit: %d -- %s\n", sec, nameStr);
    CreateFile1(sec, nameStr, buff, fileSize);
    free(buff);

    if (sec != 19) {
        char* tempP = strrchr(completeDir, '/');
        *tempP = 0;
        changeTime(sec, completeDir, nameStr);
    }
}
void help() {
    printf("ls: List directory contents\n");
    printf("cd: Change the shell working directory\n");
    printf("rm: Remove files\n");
    printf("rd: Remove files or directories\n");
    printf("mkdir: Make directories\n");
    printf("tree: show the directory tree\n");
    printf("type: show the file content\n");
    printf("touch: create files\n");
    printf("edit: edit files\n");
    printf("clr: clean the screen\n");
    printf("copy: append the second file to the first one\n");
}
void touch() {
    if (commandBuf[6] == '.') {
        //cd ..的情况
        if (commandBuf[7] == '.' && strlen(commandBuf) == 5) {
            printf("you can't create ..\n");
            return;
        }
        //cd . 的情况
        if (strlen(commandBuf) == 7) {
            printf("you can't create .\n");
            return;
        }            
    }
    //cd ''空的情况
    if (strlen(commandBuf) < 7) {
        printf("please enter a valid name\n");
        return;
    }
    //64只是比较懒，实际上应与dir统一
    char* completeDir = (char*)malloc(64);
    memset(completeDir, 0, 64);
    memcpy(completeDir, currentDir, strlen(currentDir));
    strcat(completeDir, "/");
    strcat(completeDir, commandBuf+6);

    char* nameStr = getLastName(commandBuf+6);
    nameStr[-1] = 0;
    //其他情况
    int sec;
    //确认前面路径正确
    if (commandBuf+6 != nameStr) {
        if (sec = checkDir(commandBuf+6, currentSec, 0)) {

        } else {
            printf("No such file or directory\n");
            return;
        }
    } else {
        sec = currentSec;
    }
    //检查是否重名
    if (!checkName(sec, nameStr)) {
        printf("File already exists");
        return;
    }
    //CreateFile1用于建立文件
    if (CreateFile1(sec, nameStr, NULL, 0))  {
        if (sec != 19) {
            char* tempP = strrchr(completeDir, '/');
            *tempP = 0;
            changeTime(sec, completeDir, nameStr);
        }
        return;
    } 
    printf("No such file or directory\n");
}
//only work on ANSI terminals demands POSIX
void clr() {
    printf("\e[1;1H\e[2J");
}
void waitCommand() {
    printName();
    memset(commandBuf, 0, 100);
    fgets(commandBuf, 100, stdin);
    commandBuf[strlen(commandBuf)-1] = 0;
    if (!strncmp(commandBuf, "ls", 2))
        ls();
    else if (!strncmp(commandBuf, "cd", 2))
        cd();
    else if (!strncmp(commandBuf, "rm", 2)) //rm删除文件，rd删除子目录
        rm();
    else if (!strncmp(commandBuf, "rd", 2)) 
        rd();
    else if (!strncmp(commandBuf, "mkdir", 5))
        mkdir();
    else if (!strncmp(commandBuf, "tree", 4))
        tree();
    else if (!strncmp(commandBuf, "type", 4))
        type();
    else if (!strncmp(commandBuf, "touch", 5))
        touch();
    else if (!strncmp(commandBuf, "edit", 4))
        edit();
    else if (!strncmp(commandBuf, "clr", 3))
        clr();
    else if (!strncmp(commandBuf, "copy", 3))
        copy();
    else if (!strncmp(commandBuf, "help", 6))
        help();
    //....
    else
        printf("command not found. Type 'help' to see the list.\n");
}

int main() {
    // char diskName[DISK_NAME_MAX] = {0};
    // printf("please input the file name: \n");
    // scanf("%s", diskName);
    char diskName[] = "dossys.img";
    if (OpenFAT12File(diskName)) {
        printf("success!");
    } else {
        printf("cannot open the file");
        return 0;
    }
    currentSec = 19;
    if (!initialize())
        return 0;
    while(1) {
        waitCommand();
    }

}


