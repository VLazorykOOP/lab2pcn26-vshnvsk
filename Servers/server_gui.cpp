#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h> 

#pragma comment(lib, "ws2_32.lib")

#define WM_SOCKET (WM_USER + 1)
#define ID_LISTBOX 101
#define ID_BTN_CLOSE 102

HWND hListBox;
HFONT hFont; // Глобальна змінна для шрифту
SOCKET serverSocket = INVALID_SOCKET;
SOCKET clientSocket = INVALID_SOCKET;
std::ofstream outFile;
int totalBytesReceived = 0;

void LogMessage(const wchar_t* msg) {
    SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)msg);
    int count = SendMessageW(hListBox, LB_GETCOUNT, 0, 0);
    SendMessageW(hListBox, LB_SETTOPINDEX, count - 1, 0);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Створюємо сучасний шрифт, який підтримує кирилицю
            hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            // Створюємо ListBox (трохи менший по висоті, щоб влізла кнопка)
            hListBox = CreateWindowW(L"LISTBOX", NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
                10, 10, 460, 300, hwnd, (HMENU)ID_LISTBOX, NULL, NULL);
            
            // Застосовуємо шрифт до ListBox
            SendMessageW(hListBox, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Створюємо кнопку "Закрити"
            HWND hBtnClose = CreateWindowW(L"BUTTON", L"Закрити",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                180, 320, 120, 30, hwnd, (HMENU)ID_BTN_CLOSE, NULL, NULL);
            
            // Застосовуємо шрифт до кнопки
            SendMessageW(hBtnClose, WM_SETFONT, (WPARAM)hFont, TRUE);

            LogMessage(L"Сервер запускається...");
            break;
        }
        case WM_COMMAND: {
            // Обробка натискання кнопки
            if (LOWORD(wParam) == ID_BTN_CLOSE) {
                PostMessage(hwnd, WM_CLOSE, 0, 0); // Надсилаємо команду на закриття вікна
            }
            break;
        }
        case WM_SOCKET: {
            if (WSAGETSELECTERROR(lParam)) {
                LogMessage(L"Помилка мережі!");
                return 0;
            }
            
            switch (WSAGETSELECTEVENT(lParam)) {
                case FD_ACCEPT: {
                    clientSocket = accept(wParam, NULL, NULL);
                    if (clientSocket != INVALID_SOCKET) {
                        LogMessage(L"Клієнт підключився (подія FD_ACCEPT)!");
                        WSAAsyncSelect(clientSocket, hwnd, WM_SOCKET, FD_READ | FD_CLOSE);
                        
                        outFile.open("out_gui.txt", std::ios::binary);
                        totalBytesReceived = 0;
                    }
                    break;
                }
                case FD_READ: {
                    char buffer[1024];
                    int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
                    if (bytes > 0) {
                        outFile.write(buffer, bytes);
                        totalBytesReceived += bytes;
                        
                        wchar_t logBuf[128];
                        swprintf(logBuf, 128, L"Отримано фрагмент: %d байт (Всього зібрано: %d)", bytes, totalBytesReceived);
                        LogMessage(logBuf);
                    }
                    break;
                }
                case FD_CLOSE: {
                    LogMessage(L"Клієнт відключився (подія FD_CLOSE). Прийом завершено.");
                    if (outFile.is_open()) outFile.close();
                    closesocket(clientSocket);
                    clientSocket = INVALID_SOCKET;

                    LogMessage(L"=== ВАРІАНТ 1: Відкриття отриманого файлу *.txt ===");
                    ShellExecuteA(NULL, "open", "out_gui.txt", NULL, NULL, SW_SHOWNORMAL);
                    break;
                }
            }
            break;
        }
        case WM_DESTROY: {
            if (outFile.is_open()) outFile.close();
            if (clientSocket != INVALID_SOCKET) closesocket(clientSocket);
            if (serverSocket != INVALID_SOCKET) closesocket(serverSocket);
            WSACleanup();
            DeleteObject(hFont); // Звільняємо пам'ять від створеного шрифту
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"AsyncServerClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    // Збільшив висоту вікна до 400, щоб точно влізла кнопка
    HWND hwnd = CreateWindowW(L"AsyncServerClass", L"Асинхронний Сервер - Варіант 1 (*.txt)",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&addr, sizeof(addr));
    listen(serverSocket, SOMAXCONN);

    WSAAsyncSelect(serverSocket, hwnd, WM_SOCKET, FD_ACCEPT);
    LogMessage(L"Очікування підключень на порту 5000 (WSAAsyncSelect)...");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}