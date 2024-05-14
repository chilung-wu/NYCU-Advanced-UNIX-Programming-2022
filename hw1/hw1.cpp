#include <iostream>
#include <iomanip> //setw
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <cstring> //strlen
#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h> //readlink
using namespace std;
#define buf_size 4096

// struct stat {
//                dev_t     st_dev;         /* ID of device containing file */
//                ino_t     st_ino;         /* Inode number */
//                mode_t    st_mode;        /* File type and mode */
//                nlink_t   st_nlink;       /* Number of hard links */
//                uid_t     st_uid;         /* User ID of owner */
//                gid_t     st_gid;         /* Group ID of owner */
//                dev_t     st_rdev;        /* Device ID (if special file) */
//                off_t     st_size;        /* Total size, in bytes */
//                blksize_t st_blksize;     /* Block size for filesystem I/O */
//                blkcnt_t  st_blocks;      /* Number of 512B blocks allocated */
// }

// struct dirent {
// ino_t          d_ino;       /* inode number */
// off_t          d_off;       /* offset to the next dirent */
// unsigned short d_reclen;    /* length of this record */
// unsigned char  d_type;      /* type of file */
// char           d_name[256]; /* filename */
// };


struct info{
    string cmd;
    string pid;
    string user;
    string fd;
    string type;
    string node;
    string name;
};

void Print(struct info proc_info){
    cout<<left;
    cout<<setw(20)<<proc_info.cmd;
    cout<<setw(10)<<proc_info.pid;
    cout<<setw(20)<<proc_info.user;
    cout<<setw(10)<<proc_info.fd;
    cout<<setw(10)<<proc_info.type;
    cout<<setw(10)<<proc_info.node;
    cout<<setw(30)<<proc_info.name<<endl;
}


void error(string msg, int error_no){
    cerr<<"Fail to "<<msg<<", "<<strerror(error_no)<<endl;
    exit(EXIT_FAILURE);
}


// Fix segmentation fault
// 定義一個函數來獲取文件描述符的信息
string get_fdinfo(string dir){
    FILE *fp;  // 文件指標，用於打開和讀取文件
    char last_char;  // 用於儲存解析出來的最後一個有效字符
    size_t len = 0;  // 用於 getline 函數的緩衝區長度
    char *line = NULL;  // 用於儲存 getline 讀取的每一行數據
    string result = "";  // 預設結果為空字符串，如果沒有找到匹配，將返回空字符串

    // 嘗試打開指定的文件
    if ((fp = fopen(dir.c_str(), "r")) == NULL) {
        // 如果文件無法打開，直接返回一個空字符串
        return result;
    }

    // 使用 getline 循環讀取文件的每一行
    while (getline(&line, &len, fp) != -1) {
        // 檢查當前行是否包含 "flags" 字樣
        if (string(line).find("flags") != string::npos) {
            // the last one bit is \n, the second last bit is wanted, so -2 
            // 從包含 "flags" 的行中取得倒數第二個字符（考慮到行尾可能有換行符）
            last_char = line[string(line).length() - 2];
            // 根據取得的字符判斷文件描述符的訪問權限
            switch (last_char) {
                case '0':
                    result = "r";  // 只讀
                    break;
                case '1':
                    result = "w";  // 只寫
                    break;
                case '2':
                    result = "u";  // 可讀寫
                    break;
                default:
                    break;  // 如果是其他字符，不修改 result
            }
            free(line); // 釋放 getline 分配的內存
            fclose(fp); // 已經找到所需信息，關閉文件
            return result; // 返回結果並結束函數
        }
    }

    // 如果循環結束未找到 "flags"，釋放可能未釋放的資源
    if (line != NULL) {
        free(line);
    }
    fclose(fp);  // 關閉文件
    return result;  // 返回空字符串，表示未找到 flags
}

// old version, has bug, segmentation fault.
// 函數主要用於讀取並解析文件描述符的資訊文件，以確定文件描述符的訪問權限（例如只讀、只寫或讀寫）
// 定義一個函數，該函數根據給定的目錄路徑讀取文件描述符資訊
/*
string get_fdinfo(string dir){
    FILE *fp;  // 定義一個文件指標，用於打開和讀取文件
    char last_char;  // 用於存儲解析行中的最後一個字符
    size_t len = 0;  // 用於 getline 函數的長度參數
    char *line = NULL;  // 用於存儲每行讀取的內容

    // 嘗試打開指定路徑的文件，如果打開失敗，檢查錯誤原因並返回空字符串
    if((fp = fopen(dir.c_str(), "r")) == NULL){
        // 如果文件不存在或無法訪問，返回空字符串
        if(errno == ENOENT || errno == EACCES)
            return "";
    }

    // 使用 getline 函數讀取文件的每一行
    while(getline(&line, &len, fp) != -1){
    // while(getline(&line, &len, fp) != EOF){
        // 查找包含 "flags" 的行，因為這行包含文件描述符的狀態信息
        if(string(line).find("flags") != string::npos){
            // the last one bit is \n, the second last bit is wanted, so -2 
            // 獲取行內容的倒數第二個字符，這個位置的字符表示文件描述符的狀態
            last_char = (string(line)[string(line).length() - 2]);
            // 根據字符返回相應的文件訪問權限
            if(last_char == '0'){
                return string("r");  // 如果是 '0'，表示文件是只讀的
            }
            else if(last_char == '1'){
                return string("w");  // 如果是 '1'，表示文件是只寫的
            }
            else if(last_char == '2'){
                return string("u");  // 如果是 '2'，表示文件是可讀寫的
            }
        }
    }
    free(line); // 釋放內存
    line = NULL; // 重置指針
    // free
    fclose(fp);  // 關閉文件

    // 如果文件中沒有 "flags" 行或沒有匹配的狀態，返回空字符串
    return "";
}
*/

// 定義函數以獲取指定目錄中的命令名稱
string get_cmd(string dir, string cmd_regex, string cmd){
    FILE *fp;  // 定義一個文件指針用於打開和讀取文件
    int line_byte;  // 用於存儲 getline 函數返回的讀取字節數
    size_t len = 0;  // getline 函數所需的緩衝區長度變數
    int pos;  // 用於存儲字符串查找的位置
    char *line = NULL;  // 指向動態分配線的指針，存儲讀取的行

    // 嘗試打開文件，如果失敗，根據錯誤類型返回原始 cmd
    if((fp=fopen(dir.c_str(),"r"))==NULL){
        if(errno==ENOENT || errno==EACCES)
        return cmd;  // 文件不存在或訪問被拒
    }

    // stdio getline malloc
    // 使用 getline 讀取一行文本，如果讀取失敗，調用 error 函數處理錯誤
    if((line_byte=getline(&line, &len, fp))<0){
        error("getline", errno);
    }
    fclose(fp);  // 讀取完成後關閉文件

    // cmd filter
    // 使用正則表達式進行過濾，如果找到匹配，更新 cmd
    if((pos=string(line).find(cmd_regex))!=string::npos){
        cmd = string(line);
        return cmd;
    }
    return cmd;  // 返回更新後或原始的命令名稱
}

// 定義函數以獲取進程的當前工作目錄
void get_cwd(string dir, string file_desc, struct info &proc_info, string file_regex, string type_regex){
    int buf_byte;  // 讀取字節數
    int pos;  // 字符串操作中的位置
    int node;  // 未使用的變量
    char buf[buf_size];  // 讀取鏈接目標的緩衝區
    struct stat statbuf;  // 文件狀態信息
    dir = dir + "/" + file_desc;  // 拼接完整路徑，例如 /proc/[pid]/cwd

    // 設置 proc_info 結構的文件描述符字段
    proc_info.fd = file_desc;

    // string type, name;
    // soft link
    // 嘗試讀取符號鏈接
    if((buf_byte=readlink(dir.c_str(), buf, buf_size - 1))<0){
         if(errno==EACCES){
             // 如果訪問被拒，設置類型為未知並列印
             proc_info.type = "unknown";
             proc_info.name = dir + " (Permission denied)";
             proc_info.node = "";
             Print(proc_info);
         }
         else if(errno==ENOENT)
             return;  // 鏈接不存在
         else
             error("readlink", errno);  // 處理其他錯誤
    }
    else{
        buf[buf_byte] = '\0'; // Ensure null termination
        // string s;
        // s.substr(0, buf_byte);
        // 獲取鏈接指向的文件狀態
        if(stat(dir.c_str(), &statbuf)<0){
            error("lstat", errno);
        }
        // 設置類型為目錄，節點號，並從讀取的數據終止字符串
        proc_info.type = "DIR";
        proc_info.node = to_string(statbuf.st_ino);
        buf[buf_byte] = '\0';
        proc_info.name = buf;
        // 檢查是否符合過濾條件並列印
        if((pos=proc_info.name.find(file_regex))!=string::npos && (pos=proc_info.type.find(type_regex))!=string::npos)
            Print(proc_info);
    }
}


// 此函數用於讀取進程的根目錄 (root directory) 的符號鏈接
void get_rtd(string dir, string file_desc, struct info &proc_info, string file_regex, string type_regex){
    int buf_byte;  // 讀取字節數
    int pos;  // 字符串操作中的位置
    char buf[buf_size];  // 讀取鏈接目標的緩衝區
    struct stat statbuf;  // 文件狀態信息
    dir = dir + "/" + file_desc;  // 拼接完整路徑，例如 /proc/[pid]/root
    proc_info.fd = "rtd";
    
    //soft link
    // 嘗試讀取符號鏈接
    if ((buf_byte = readlink(dir.c_str(), buf, buf_size - 1)) < 0) {
        if (errno == EACCES) {  // 訪問被拒
            proc_info.type = "unknown";
            proc_info.name = dir + " (Permission denied)";
            proc_info.node = "";
            Print(proc_info);
        } else if (errno == ENOENT)  // 鏈接不存在
            return;
        else  // 其他錯誤
            error("readlink", errno);
    } else {
        // 獲取鏈接指向的文件狀態
        if (stat(dir.c_str(), &statbuf) < 0) {
            error("lstat", errno);
        }
        buf[buf_byte] = '\0'; // Ensure null termination
        proc_info.type = "DIR";  // 目錄類型
        proc_info.node = to_string(statbuf.st_ino);  // 節點號
        buf[buf_byte] = '\0';  // 結束字符串
        proc_info.name = buf;  // 文件名或路徑
        // 檢查是否符合過濾條件並列印
        if ((pos = proc_info.name.find(file_regex)) != string::npos && (pos = proc_info.type.find(type_regex)) != string::npos)
            Print(proc_info);
    }
}

// 此函數用於讀取進程的執行檔路徑的符號鏈接
void get_txt(string dir, string file_desc, struct info &proc_info, string file_regex, string type_regex){
    int buf_byte;  // 讀取字節數
    int pos;  // 字符串操作中的位置
    char buf[buf_size];  // 讀取鏈接目標的緩衝區
    struct stat statbuf;  // 文件狀態信息
    dir = dir + "/" + file_desc;  // 拼接完整路徑，例如 /proc/[pid]/exe
    proc_info.fd = "txt";

    //soft link
    // 嘗試讀取符號鏈接
    if ((buf_byte = readlink(dir.c_str(), buf, buf_size - 1)) < 0) {
        if (errno == EACCES) {  // 訪問被拒
            proc_info.type = "unknown";
            proc_info.name = dir + " (Permission denied)";
            proc_info.node = "";
            Print(proc_info);
        } else if (errno == ENOENT)  // 鏈接不存在
            return;
        else  // 其他錯誤
            error("readlink", errno);
    } else {
        buf[buf_byte] = '\0'; // Ensure null termination
        // 獲取鏈接指向的文件狀態
        if (stat(dir.c_str(), &statbuf) < 0) {
            error("lstat", errno);
        }
        // 根據文件類型設置 type
        switch (statbuf.st_mode & S_IFMT) {
            case S_IFCHR:  proc_info.type = "CHR";        break;
            case S_IFDIR:  proc_info.type = "DIR";        break;
            case S_IFIFO:  proc_info.type = "FIFO";       break;
            case S_IFREG:  proc_info.type = "REG";        break;
            case S_IFSOCK: proc_info.type = "SOCK";       break;
            default:       proc_info.type = "unknown";    break;
        }
        proc_info.node = to_string(statbuf.st_ino);  // 節點號
        buf[buf_byte] = '\0';  // 結束字符串
        proc_info.name = buf;  // 文件名或路徑
        // 檢查是否符合過濾條件並列印
        if ((pos = proc_info.name.find(file_regex)) != string::npos && (pos = proc_info.type.find(type_regex)) != string::npos)
            Print(proc_info);
    }
}

//file
//address  permit   offset  device  node  name
// 定義函數，傳入目錄路徑、描述符（通常是 'maps'），proc_info 結構體，以及文件和類型的正則表達式。
void get_mem(string dir, string file_desc, struct info &proc_info, string file_regex, string type_regex){
    FILE *fp;  // 用於打開文件的指針
    dir = dir + "/" + file_desc;  // 構造完整的文件路徑，例如 /proc/[pid]/maps
    struct stat statbuf;  // 用於儲存文件狀態信息的結構體
    size_t len = 0;  // getline 函數所需的緩衝區大小變數
    char *line = NULL;  // 存儲從文件中讀取的每一行的指針

    // 嘗試打開文件，如果失敗，根據錯誤類型可能直接返回
    if ((fp = fopen(dir.c_str(), "r")) == NULL) {
        if (errno == ENOENT || errno == EACCES)
            return;
    }
    // 獲取文件的狀態信息，如果失敗，調用 error 函數處理錯誤
    if (stat(dir.c_str(), &statbuf) < 0)
        error("stat", errno);

    char *chunk;  // 用於分割字符串的臨時指針
    string address = "", permit = "", offset = "", device = "";  // 存儲分割後的各部分信息
    string check_delete = "";  // 標記文件是否被刪除
    // 逐行讀取文件內容
    // while (getline(&line, &)
    // while (getline(&line, &len, fp) < 0))
    while (getline(&line, &len, fp) != EOF) {
        proc_info.fd = "mem";  // 標記為記憶體映射類型
        if ((chunk = strtok(line, " ")) != NULL)
            address = string(chunk);
        if ((chunk = strtok(NULL, " ")) != NULL)
            permit = string(chunk);
        if ((chunk = strtok(NULL, " ")) != NULL)
            offset = string(chunk);
        if ((chunk = strtok(NULL, " ")) != NULL)
            device = string(chunk);
        if ((chunk = strtok(NULL, " ")) != NULL) {
            // only need to output the first one for duplicated files, i.e., files having the same ! i-node ! or filename.  empty-> node is 0.
            // 檢查節點是否重複或為空，避免重複輸出
            if (proc_info.node == string(chunk) || string(chunk) == "0")
                continue;
            proc_info.node = string(chunk);
        }
        if ((chunk = strtok(NULL, " ")) != NULL)
            proc_info.name = string(chunk);
        // 移除名稱中的換行符
        int pos;
        if ((pos = proc_info.name.find("\n")) != string::npos) {
            proc_info.name = proc_info.name.replace(pos, 1, "");
        }
        // 檢查是否標記為已刪除
        if ((chunk = strtok(NULL, " ")) != NULL) {
            check_delete = string(chunk);
            int length = strlen(check_delete.c_str());
            check_delete.replace(length - 1, 1, "");
        }
        if (check_delete.find("deleted") != string::npos) {
            proc_info.fd = "del";  // 標記為已刪除
            proc_info.name += " (deleted)";
        }
        // 根據 stat 結構體判斷文件類型
        switch (statbuf.st_mode & S_IFMT) {
            case S_IFCHR:  proc_info.type = "CHR";        break;
            case S_IFDIR:  proc_info.type = "DIR";        break;
            case S_IFIFO:  proc_info.type = "FIFO";       break;
            case S_IFREG:  proc_info.type = "REG";        break;
            case S_IFSOCK: proc_info.type = "SOCK";       break;
            default:       proc_info.type = "unknown";    break;
        }
        // 判斷名稱和類型是否符合正則表達式，如果符合則列印
        if ((pos = proc_info.name.find(file_regex)) != string::npos && (pos = proc_info.type.find(type_regex)) != string::npos)
            Print(proc_info);
    }
    // 釋放內存
    free(line);
    line = NULL;
    // 關閉文件指針
    fclose(fp);
}

//dir
// 定義函數，用於獲取文件描述符資訊
void get_fd(string dir, string file_desc, struct info &proc_info, string file_regex, string type_regex){
    // 建立完整的目錄路徑
    string dir_path = dir + "/" + file_desc;  // 例: /proc/[pid]/fd
    char buf[buf_size];  // 儲存讀取到的鏈結目標
    int buf_byte;  // 讀取的字節數
    DIR *dp;  // 目錄流指針
    struct dirent *dirp;  // 目錄項結構體指針
    struct stat statbuf;  // 文件狀態信息結構體

    // 嘗試打開目錄
    if ((dp = opendir(dir_path.c_str())) == NULL) {
        // 如果無法訪問，可能是權限問題
        if (errno == EACCES) {
            // 設定 proc_info 結構體並列印
            proc_info.fd = "NOFD";
            proc_info.type = "";
            proc_info.name = dir_path + " (Permission denied)";
            Print(proc_info);
            return;
        }
    }

    // 讀取目錄內的每個條目
    while ((dirp = readdir(dp)) != NULL) {
        // 確保條目名是數字（文件描述符通常是數字）
        if (isdigit(dirp->d_name[0])) {
            // 建立 /fdinfo/ 路徑
            string fdinfopath = dir + "/fdinfo/";
            // 構建文件描述符的具體路徑
            string sflink = dir_path + "/" + string(dirp->d_name); // proc/[pid]/fd/[fd]

            // 嘗試讀取文件描述符指向的實際文件（符號鏈結）
            if ((buf_byte = readlink(sflink.c_str(), buf, buf_size - 1)) < 0) {
                if (errno == ENOENT)
                    return;
                else
                    error("readlink", errno);
            } else {
                buf[buf_byte] = '\0'; // Ensure null termination
            }

            // 獲取文件的狀態
            if (stat(sflink.c_str(), &statbuf) < 0) {
                error("lstat", errno);
            }

            // 判斷文件類型
            switch (statbuf.st_mode & S_IFMT) {
                case S_IFCHR:  proc_info.type = "CHR";        break;
                case S_IFDIR:  proc_info.type = "DIR";        break;
                case S_IFIFO:  proc_info.type = "FIFO";       break;
                case S_IFREG:  proc_info.type = "REG";        break;
                case S_IFSOCK: proc_info.type = "SOCK";       break;
                default:       proc_info.type = "unknown";    break;
            }

            // 組裝 fdinfo 路徑
            proc_info.fd = string(dirp->d_name);
            fdinfopath += proc_info.fd;
            // 獲取文件描述符的額外資訊
            proc_info.fd += get_fdinfo(fdinfopath);
            

            /* something wrong for use access(), change to use fdinfo's flags
            // for fd r, u, w 
            int r=0, w=0;
            string i="";// i=0 read, i=1  write, i=2  both 
            if(access(sflink.c_str(), R_OK)==0){
                r=1;
            }     
            if(access(sflink.c_str(), W_OK)==0){
                w=1;
            }
            if(r==1 && w==1){
                i = "u";
            }
            else if( r==0 && w==1){
                i = "w";
            }
            else if( r==1 && w==0){
                i = "r";
            }
            else {
                i = "";
            }
            proc_info.fd += i;
            */

            // if(access(sflink.c_str(), R_OK)==0){
            //     if(access(sflink.c_str(), W_OK)==0){
            //        proc_info.fd += "u";
            //     }
            //     else 
            //         proc_info.fd += "r";
            // }
            // else if(access(sflink.c_str(), R_OK)==0){
            //     proc_info.fd += "r";
            // }
            // else if(access(sflink.c_str(), W_OK)==0){
            //     proc_info.fd += "w";
            // }


            // 記錄節點號並去除字符串尾部的 "(deleted)" 標記
            proc_info.node = to_string(statbuf.st_ino);
            buf[buf_byte] = '\0';
            proc_info.name = buf;

            /*
            if(proc_info.type=="FIFO")
                buf[buf_byte] = '\0';
                proc_info.name = buf;
                proc_info.name = "pipe:[" + proc_info.node + "]";
            else if(proc_info.type=="SOCK")
                proc_info.name = "socket:[" + proc_info.node + "]";
            else {
                buf[buf_byte]='\0';
                proc_info.name = buf;
            }
            */
            int pos;
            if((pos = proc_info.name.find("(deleted)"))!=string::npos){
                proc_info.name = proc_info.name.substr(0, pos-1);
            }
            
            // 如果文件名和類型匹配正則表達式，則列印該進程信息
            if ((pos = proc_info.name.find(file_regex)) != string::npos && (pos = proc_info.type.find(type_regex)) != string::npos)
                Print(proc_info);
        }
    }
    // 關閉目錄流
    closedir(dp);
}

//         //    man7 stat ex.  select field
//         //    switch (sb.st_mode & S_IFMT) {
//         //    case S_IFBLK:  printf("block device\n");            break;
//         //    case S_IFCHR:  printf("character device\n");        break;
//         //    case S_IFDIR:  printf("directory\n");               break;
//         //    case S_IFIFO:  printf("FIFO/pipe\n");               break;
//         //    case S_IFLNK:  printf("symlink\n");                 break;
//         //    case S_IFREG:  printf("regular file\n");            break;
//         //    case S_IFSOCK: printf("socket\n");                  break;
//         //    default:       printf("unknown?\n");                break;
//         //    }
//         switch (statbuf.st_mode & S_IFMT) {
//             case S_IFCHR:  proc_info.type = "CHR";        break;
//             case S_IFDIR:  proc_info.type = "DIR";        break;
//             case S_IFIFO:  proc_info.type = "FIFO";       break;
//             case S_IFREG:  proc_info.type = "REG";        break;
//             case S_IFSOCK: proc_info.type = "SOCK";       break;
//             default:       proc_info.type = "unknown";    break;
//         }
// }

// 根據給定目錄的文件狀態獲取用戶名
string get_user(string dir){
    // struct passwd {
    //            char   *pw_name;       /* username */
    //            char   *pw_passwd;     /* user password */
    //            uid_t   pw_uid;        /* user ID */
    //            gid_t   pw_gid;        /* group ID */
    //            char   *pw_gecos;      /* user information */
    //            char   *pw_dir;        /* home directory */
    //            char   *pw_shell;      /* shell program */
    //        };
    struct passwd *pws;  //pwd.h // 存儲用戶信息的結構體
    struct stat statbuff;  // 存儲文件狀態信息

    // 獲取文件狀態
    if (stat(dir.c_str(), &statbuff) < 0)
        error("stat", errno);

    // 根據用戶ID獲取用戶信息結構體
    if ((pws = getpwuid(statbuff.st_uid)) == NULL)
        error("getpwuid", errno);

    // 返回用戶名
    return pws->pw_name;
}


// void selec_pid()

// main 函數：程序的進入點，接收命令列參數並執行程式的主要邏輯。
int main(int argc, const char* argv[]){
    // 宣告一個指向 DIR 結構的指標，用於打開和閱讀目錄。
    DIR *dp; // opendir
    // dirent 結構的指標，用於讀取目錄中的每個檔案/目錄的信息。
    struct dirent *dirp; // readdir
    // 字串 proc_dir 存儲了要探索的目錄路徑，通常指向 Linux 系統的 /proc 目錄，用於訪問系統進程信息。
    string proc_dir="/proc/";
    // -c REGEX, -t TYPE, -f REGEX 這些變量用於儲存通過命令列參數指定的過濾條件。
    string cmd_regex = "", type_regex = "", file_regex = "";

    // 解析命令列參數。
    for(int i=1; i<argc ; i=i+2){
        // 檢查 "-c" 選項，用於指定過濾進程名稱的正則表達式。
        if(strcmp(argv[i], "-c")==0){
            cmd_regex = argv[i+1];
        }
        // 檢查 "-t" 選項，用於指定過濾文件類型。
        else if(strcmp(argv[i], "-t")==0){
            type_regex = string(argv[i+1]);
            // Valid TYPE includes REG, CHR, DIR, FIFO, SOCK, and unknown
            // 如果提供的類型不是預定義的有效類型之一，則輸出錯誤信息並終止程序。
            if(type_regex!="REG" && type_regex!="CHR" && type_regex!="DIR" && type_regex!="FIFO" && type_regex!="SOCK" && type_regex!="unknown"){
                cerr<<"Invalid TYPE option.\n";
                exit(1);
            }
        }
        // 檢查 "-f" 選項，用於指定過濾文件名的正則表達式。
        else if(strcmp(argv[i], "-f")==0){
            file_regex = string(argv[i+1]);
        }
    }

    // 嘗試打開 /proc 目錄。
    if((dp = opendir(proc_dir.c_str()))== NULL){
        // 如果打開失敗，則調用 error 函數處理錯誤並終止程序。
        error("opendir", errno);
    }

    // 列印表頭，顯示不同屬性的標題。
    cout<<"COMMAND         PID             USER            FD              TYPE            NODE            NAME         "<<endl;

    // 讀取 /proc 目錄下的每個條目。
    while((dirp = readdir(dp))!= NULL){
        // 檢查條目名稱是否為數字開頭，因為在 /proc 目錄下，數字名稱的子目錄對應於系統中的進程。
        if(isdigit(dirp->d_name[0])){
            string cmd = "";
            struct info proc_info;

            // 獲取進程的命令名稱。
            proc_info.cmd = get_cmd(proc_dir + string(dirp->d_name) + "/comm", cmd_regex, cmd);

            // if(proc_info.cmd.compare("a.out")==0){
            // if(proc_info.cmd == "a.out"){
            //     cout<<"___"<<endl;
            //     break;
            // }

            // 如果命令名稱為空，則跳過這個進程。
            if(strcmp(proc_info.cmd.c_str(), "")==0){
                continue;
            }
            else {
                // delete cmd tail \n 刪除命令名稱字串尾部的換行符。
                proc_info.cmd = proc_info.cmd.substr(0, proc_info.cmd.find("\n"));
                // 設置 proc_info 結構的 pid 屬性。
                proc_info.pid = string(dirp->d_name);
                // 獲取當前進程的 PID，用於過濾自身進程。
                int p = getpid();
                if(proc_info.pid == to_string(p)){
                    continue;
                }

                // 獲取進程擁有者的用戶名。
                proc_info.user = get_user(proc_dir + proc_info.pid + "/comm" );

                // softlink 處理軟鏈接和相關文件信息。
                get_cwd(proc_dir+proc_info.pid,"cwd", proc_info, file_regex, type_regex);
                get_rtd(proc_dir+proc_info.pid,"root", proc_info, file_regex, type_regex);
                get_txt(proc_dir+proc_info.pid,"exe", proc_info, file_regex, type_regex);

                // each item is file. 處理進程映射到記憶體中的文件。
                get_mem(proc_dir+proc_info.pid,"maps", proc_info, file_regex, type_regex);
                // each item is dir. 處理進程打開的文件描述符。
                get_fd(proc_dir+proc_info.pid,"fd", proc_info, file_regex, type_regex);
            }
        }
    }
    // 程式正常結束。
    return 0;
}


// int main(int argc, const char* argv[]){
//     DIR *dp; //opendir
//     struct dirent *dirp; //readdir
//     string proc_dir="/proc/";
//     // -c REGEX, -t TYPE, -f REGEX
//     string cmd_regex = "", type_regex = "", file_regex = "";
//     //deal argv
//     for(int i=1; i<argc ; i=i+2){
//         if(strcmp(argv[i], "-c")==0){
//             cmd_regex = argv[i+1];
//             // cout<<cmd_regex<<" +++"<<endl;
//         }
//         else if(strcmp(argv[i], "-t")==0){
//             type_regex = string(argv[i+1]);
//             // cout<<type_regex<<" +++"<<endl;
//             // Valid TYPE includes REG, CHR, DIR, FIFO, SOCK, and unknown
//             if(type_regex!="REG" && type_regex!="CHR" && type_regex!="DIR" && type_regex!="FIFO" && type_regex!="SOCK" && type_regex!="unknown"){
//                 cerr<<"Invalid TYPE option.\n";
//                 exit(1);
//             }
//             // cout<<type_regex<<" OK "<<endl;
//         }
//         else if(strcmp(argv[i], "-f")==0){
//             file_regex = string(argv[i+1]);
//             // cout<<file_regex<<" +++"<<endl;
//         }
//     }

//     if((dp = opendir(proc_dir.c_str()))== NULL){
//         error("opendir", errno);
//     }
//     //COMMAND         PID             USER            FD              TYPE            NODE            NAME
//     cout<<"COMMAND         PID             USER            FD              TYPE            NODE            NAME         "<<endl;

//     while((dirp = readdir(dp))!= NULL){
//         if(isdigit(dirp->d_name[0])){
//             string cmd = "";
//             struct info proc_info;
//             // cat comm
//             proc_info.cmd = get_cmd(proc_dir + string(dirp->d_name) + "/comm", cmd_regex, cmd);
//             // if(proc_info.cmd.compare("a.out")==0){
//             // if(proc_info.cmd == "a.out"){
//             //     cout<<"___"<<endl;
//             //     break;
//             // }
//             if(strcmp(proc_info.cmd.c_str(), "")==0){
//                 continue;
//             }
//             else {
//                 // cout<<proc_info.cmd<<endl;

//                 //delete cmd tail \n
//                 proc_info.cmd = proc_info.cmd.substr(0, proc_info.cmd.find("\n"));
//                 proc_info.pid = string(dirp->d_name);
//                 int p = getpid();
//                 if(proc_info.pid == to_string(p)){
//                     continue;
//                 }
//                 // cout<<proc_info.cmd<<endl;
//                 proc_info.user = get_user(proc_dir + proc_info.pid + "/comm" );

//                 //softlink
//                 get_cwd(proc_dir+proc_info.pid,"cwd", proc_info, file_regex, type_regex);
//                 get_rtd(proc_dir+proc_info.pid,"root", proc_info, file_regex, type_regex);
//                 get_txt(proc_dir+proc_info.pid,"exe", proc_info, file_regex, type_regex);

//                 //file
//                 get_mem(proc_dir+proc_info.pid,"maps", proc_info, file_regex, type_regex);
//                 //dir
//                 get_fd(proc_dir+proc_info.pid,"fd", proc_info, file_regex, type_regex);

//                 // cout<<proc_info.cmd<<" "<<proc_info.pid<<" "<<proc_info.user<<endl;

//             }
//         }
//     }
//     return 0;
// }

/*
pw_name
User name
pw_uid
User ID (UID) number
pw_gid
Group ID (GID) number
pw_dir
Initial working directory
pw_shell
Initial user program
*/