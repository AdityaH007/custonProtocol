#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib

//declared the constants of the frame 
const std::string START_IDENTIFIER = "START";
const std::string END_IDENTIFIER = "END";
const std::string CREATOR_NAME = "aditya";

// Function to convert string to binary representation
std::vector<uint8_t> stringToBinary(const std::string& str) {
    std::vector<uint8_t> binary;
    for (char c : str) {
        for (int i = 7; i >= 0; --i) {
            binary.push_back((c >> i) & 1);
        }
    }
    return binary;
}

// Function to create the message frame
std::vector<uint8_t> createMessageFrame(const std::string& country, const std::string& text) {
    std::vector<uint8_t> frame;

    // Start Identifier
    auto startBinary = stringToBinary(START_IDENTIFIER);
    frame.insert(frame.end(), startBinary.begin(), startBinary.end());

    // Creator Name
    auto creatorBinary = stringToBinary(CREATOR_NAME);
    frame.insert(frame.end(), creatorBinary.begin(), creatorBinary.end());

    // Message (Country and Text)
    std::string message = country + "," + text;
    auto messageBinary = stringToBinary(message);
    frame.insert(frame.end(), messageBinary.begin(), messageBinary.end());

    // End Identifier
    auto endBinary = stringToBinary(END_IDENTIFIER);
    frame.insert(frame.end(), endBinary.begin(), endBinary.end());

    return frame;
}


void printBinaryFrame(const std::vector<uint8_t>& frame) {
    std::cout << "Binary Frame: ";
    for (auto bit : frame) {
        std::cout << (int)bit;
    }
    std::cout << std::endl;
}


void printAsciiFrame(const std::vector<uint8_t>& frame, const std::string& country, const std::string& text) {
    int startSize = START_IDENTIFIER.size() * 8;
    int creatorSize = CREATOR_NAME.size() * 8;
    int messageSize = (country.size() + text.size() + 1) * 8;  // +1 for commaaaaaaaaa
    int endSize = END_IDENTIFIER.size() * 8;

    std::cout << " +------------------------------------------------------+\n";
    std::cout << " | Frame (ASCII Representation)                          |\n";
    std::cout << " +-----------------------+------------------------------+\n";

    std::cout << " | Start Identifier:      | ";
    for (int i = 0; i < startSize; ++i) std::cout << (int)frame[i];
    std::cout << " |\n";

    std::cout << " | Creator Name:          | ";
    for (int i = startSize; i < startSize + creatorSize; ++i) std::cout << (int)frame[i];
    std::cout << " |\n";

    std::cout << " | Message (Country+Text):| ";
    for (int i = startSize + creatorSize; i < startSize + creatorSize + messageSize; ++i) std::cout << (int)frame[i];
    std::cout << " |\n";

    std::cout << " | End Identifier:        | ";
    for (int i = startSize + creatorSize + messageSize; i < startSize + creatorSize + messageSize + endSize; ++i) std::cout << (int)frame[i];
    std::cout << " |\n";

    std::cout << " +------------------------------------------------------+\n";

    // Display ASCII art frame (textual parts as well)
    std::cout << "\nFrame Structure:\n";
    std::cout << "+--------------+\n";
    std::cout << "| START: " << START_IDENTIFIER << " |\n";
    std::cout << "+--------------+\n";
    std::cout << "| CREATOR: " << CREATOR_NAME << " |\n";
    std::cout << "+--------------+\n";
    std::cout << "| MESSAGE: " << country << ", " << text << " |\n";
    std::cout << "+--------------+\n";
    std::cout << "| END: " << END_IDENTIFIER << " |\n";
    std::cout << "+--------------+\n";
}


void decodeMessageFrame(const std::vector<uint8_t>& frame) {
    // Converting binary back to text for display
    auto binaryToString = [](const std::vector<uint8_t>& binary, size_t start, size_t length) -> std::string {
        std::string result;
        for (size_t i = start; i < start + length; i += 8) {
            char c = 0;
            for (int j = 0; j < 8; ++j) {
                c = (c << 1) | binary[i + j];
            }
            result.push_back(c);
        }
        return result;
        };

    size_t startSize = START_IDENTIFIER.size() * 8;
    size_t creatorSize = CREATOR_NAME.size() * 8;
    size_t messageSize = frame.size() - (startSize + creatorSize + END_IDENTIFIER.size() * 8);

    std::string start = binaryToString(frame, 0, startSize);
    std::string creator = binaryToString(frame, startSize, creatorSize);
    std::string message = binaryToString(frame, startSize + creatorSize, messageSize);
    std::string end = binaryToString(frame, startSize + creatorSize + messageSize, END_IDENTIFIER.size() * 8);

    std::cout << "\nDecoded Frame Data:\n";
    std::cout << "Start Identifier: " << start << std::endl;
    std::cout << "Creator Name: " << creator << std::endl;
    std::cout << "Message: " << message << std::endl;
    std::cout << "End Identifier: " << end << std::endl;
}


void sendMessageToServer(SOCKET sock) {
    std::string country, text;

    while (true) {
        std::cout << "Enter country: ";
        std::getline(std::cin, country);
        std::cout << "Enter message: ";
        std::getline(std::cin, text);

        // Create the message frame
        auto messageFrame = createMessageFrame(country, text);

        // Send the message frame
        send(sock, reinterpret_cast<const char*>(messageFrame.data()), messageFrame.size(), 0);
        printBinaryFrame(messageFrame);
        printAsciiFrame(messageFrame, country, text);
        std::cout << "Message sent to server.\n";

        // Animation
        std::cout << "Sending";
        for (int i = 0; i < 3; ++i) {
            std::cout << ".";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::cout << "\n";
    }
}


void receiveMessages(SOCKET sock) {
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            // Process received data as binary
            std::vector<uint8_t> receivedFrame(buffer, buffer + bytesReceived);
            std::cout << "\nReceived Message (Binary):\n";
            printBinaryFrame(receivedFrame);

           
            decodeMessageFrame(receivedFrame);
        }
    }
}


void startServer(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    std::cout << "Server is listening on port " << port << "...\n";

    SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
    std::cout << "Client connected!\n";

    
    receiveMessages(clientSocket);

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
}

// Function to start the client
void startClient(const std::string& ip, int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);
    serverAddr.sin_port = htons(port);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        return;
    }

    std::cout << "Connected to server!\n";

    // Send messages to server
    sendMessageToServer(clientSocket);

    closesocket(clientSocket);
    WSACleanup();
}

int main() {
    int choice;
    std::cout << "1. Start Server\n2. Start Client\nEnter choice: ";
    std::cin >> choice;
    std::cin.ignore(); // Clear newline from input buffer

    if (choice == 1) {
        int port;
        std::cout << "Enter port: ";
        std::cin >> port;
        startServer(port);
    }
    else if (choice == 2) {
        std::string ip;
        int port;
        std::cout << "Enter server IP: ";
        std::cin >> ip;
        std::cout << "Enter server port: ";
        std::cin >> port;
        startClient(ip, port);
    }

    return 0;
}
