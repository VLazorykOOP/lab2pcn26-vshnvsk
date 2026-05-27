#include <iostream>
#include <fstream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "Помилка ініціалізації WinSock.\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cout << "Помилка створення сокета.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5000);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    // Переводимо сокет у неблокуючий режим для циклічного виклику accept()
    u_long iMode = 1;
    ioctlsocket(serverSocket, FIONBIO, &iMode);

    SOCKET clientSocket = INVALID_SOCKET;
    cout << "Сервер запущено. Очікування підключення клієнта\n";

    // Реалізація циклічного виклику accept() до встановлення з'єднання
    while (true) {
        clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            cout << "\n[Успіх] З'єднання з клієнтом встановлено!\n";
            break; 
        }
        
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            cout << "Помилка accept: " << err << "\n";
            break;
        }
        cout << "."; // Візуалізація циклічного очікування
        Sleep(500);  // Затримка півсекунди, щоб не перевантажувати процесор
    }

    // Повертаємо блокуючий режим для нормального прийому даних
    iMode = 0;
    ioctlsocket(clientSocket, FIONBIO, &iMode);

    // Відкриваємо файл для запису отриманих даних
    ofstream outFile("out_console.txt", ios::binary);
    char buffer[1024];
    int bytesReceived;
    int totalBytes = 0;

    cout << "Початок прийому файлу...\n";
    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        outFile.write(buffer, bytesReceived);
        totalBytes += bytesReceived;
    }

    cout << "Прийом завершено. Отримано байт: " << totalBytes << "\n";

    outFile.close();
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}