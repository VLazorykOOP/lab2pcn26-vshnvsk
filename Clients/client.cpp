#include <iostream>
#include <fstream>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    int XX = 99; // Дві останні цифри
    int mode = 2; // 2 - передача фрагментами
    const char* fileName = "text.txt"; // Файл C++

    // Створюємо справжній файл з кодом С++ для тесту
    ofstream testFile(fileName, ios::trunc);
    testFile << "Hello\n\n";
    testFile << " \n";
    testFile << "How are you?\n";
    testFile << " \n";
    testFile << "Bye\n";
    testFile.close();

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    cout << "Підключення до віконного сервера...\n";
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Помилка підключення!\n";
        return 1;
    }
    cout << "Підключено!\n";

    ifstream inFile(fileName, ios::binary | ios::ate);
    streamsize fileSize = inFile.tellg();
    inFile.seekg(0, ios::beg);
    vector<char> fileData(fileSize);
    
    if (inFile.read(fileData.data(), fileSize)) {
        cout << "Передача файлу '*.txt' у " << XX << " фрагментах...\n";
        int chunkSize = fileSize / XX;
        int remainder = fileSize % XX;
        int offset = 0;

        for (int i = 0; i < XX; ++i) {
            int currentChunkSize = chunkSize + (i == XX - 1 ? remainder : 0); 
            if (currentChunkSize > 0) {
                send(clientSocket, fileData.data() + offset, currentChunkSize, 0);
                offset += currentChunkSize;
                cout << "Відправлено фрагмент " << i + 1 << " (" << currentChunkSize << " байт)\n";
                Sleep(20); // Затримка для наочності асинхронності на сервері
            }
        }
        cout << "Файл успішно відправлено!\n";
    }

    inFile.close();
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}