/**
 * ============================================================================
 * Project Title : Real-Time Coding Contest Client
 * Course        : Operating Systems Lab (25B16CS211) - JIIT
 * Submitted By  : Akriti Singh (992401030302), Ayushi Tripathi (992401030317)
 *                 Avni Bansal  (992401030319), Malvika Saxena   (992401030325)
 * Submitted To  : Asst. Prof. Satya Prakash Patel
 *
 * Description   :
 *   Interactive participant client. Connects over TCP socket, logs in,
 *   retrieves contest problems, submits solutions to the server, and
 *   views real-time synchronized leaderboard rankings.
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <sstream>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
    inline void init_platform() {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
    }
    inline void cleanup_platform() { WSACleanup(); }
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    typedef int socket_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    inline void init_platform() {}
    inline void cleanup_platform() {}
#endif

static const char* SERVER_IP = "127.0.0.1";
static const int SERVER_PORT = 8080;

bool send_line(socket_t sock, const std::string& text) {
    std::string msg = text + "\n";
    int sent = send(sock, msg.c_str(), (int)msg.length(), 0);
    return (sent != SOCKET_ERROR);
}

bool recv_line(socket_t sock, std::string& out) {
    out.clear();
    char ch;
    while (true) {
        int r = recv(sock, &ch, 1, 0);
        if (r <= 0) return false;
        if (ch == '\n') break;
        if (ch != '\r') out += ch;
    }
    return true;
}

void print_menu(const std::string& username) {
    std::cout << "\n=======================================================\n";
    std::cout << "        REAL-TIME CODING CONTEST PARTICIPANT           \n";
    std::cout << "        Logged in as: [" << username << "]\n";
    std::cout << "=======================================================\n";
    std::cout << "  [1] View Contest Problems\n";
    std::cout << "  [2] Submit Solution to a Problem\n";
    std::cout << "  [3] View Live Synchronized Leaderboard\n";
    std::cout << "  [4] View My Current Score & Solved Count\n";
    std::cout << "  [5] Exit / Logout\n";
    std::cout << "-------------------------------------------------------\n";
    std::cout << "Select an option (1-5): ";
}

int main(int argc, char* argv[]) {
    init_platform();

    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create TCP socket.\n";
        cleanup_platform();
        return 1;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    std::cout << "Connecting to Contest Server at " << SERVER_IP << ":" << SERVER_PORT << "...\n";
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        std::cerr << "Error: Could not connect to Contest Server. Is server running?\n";
        closesocket(sock);
        cleanup_platform();
        return 1;
    }

    // Read initial server banner
    std::string server_banner;
    recv_line(sock, server_banner);
    std::cout << "[SERVER]: " << server_banner << "\n";

    // Non-interactive automated mode (if CLI arguments provided: client.exe <name> <prob_id> <code>)
    if (argc >= 4) {
        std::string student_name = argv[1];
        int prob_id = std::stoi(argv[2]);
        std::string code = argv[3];

        send_line(sock, "LOGIN " + student_name);
        std::string resp;
        recv_line(sock, resp); // Login response

        send_line(sock, "SUBMIT " + std::to_string(prob_id) + " " + code);
        recv_line(sock, resp); // Queued response
        std::cout << student_name << " -> " << resp << "\n";
        recv_line(sock, resp); // Result response
        std::cout << student_name << " -> " << resp << "\n";

        send_line(sock, "STATUS");
        recv_line(sock, resp);
        std::cout << student_name << " -> " << resp << "\n";

        send_line(sock, "EXIT");
        closesocket(sock);
        cleanup_platform();
        return 0;
    }

    // Interactive Mode
    std::string student_name;
    std::cout << "\nEnter your Name or Enrollment ID to Login: ";
    std::getline(std::cin, student_name);
    while (student_name.empty()) {
        std::cout << "Name cannot be empty. Enter your Name: ";
        std::getline(std::cin, student_name);
    }

    send_line(sock, "LOGIN " + student_name);
    std::string login_response;
    recv_line(sock, login_response);
    std::cout << "[SERVER]: " << login_response << "\n";

    int choice = 0;
    while (true) {
        print_menu(student_name);
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::string dummy;
            std::cin >> dummy;
            continue;
        }

        if (choice == 1) {
            send_line(sock, "PROBLEMS");
            std::string line;
            while (recv_line(sock, line)) {
                std::cout << line << "\n";
                if (line.find("--- END OF PROBLEMS ---") != std::string::npos) break;
            }
        }
        else if (choice == 2) {
            int prob_id;
            std::cout << "Enter Problem ID (1, 2, or 3): ";
            std::cin >> prob_id;
            std::cin.ignore();

            std::cout << "Enter your solution code snippet (one line): ";
            std::string code;
            std::getline(std::cin, code);

            send_line(sock, "SUBMIT " + std::to_string(prob_id) + " " + code);
            std::string queued_msg, result_msg;
            recv_line(sock, queued_msg);
            std::cout << "[SERVER]: " << queued_msg << "\n";
            recv_line(sock, result_msg);
            std::cout << "[SERVER]: " << result_msg << "\n";
        }
        else if (choice == 3) {
            send_line(sock, "LEADERBOARD");
            std::string line;
            while (recv_line(sock, line)) {
                std::cout << line << "\n";
                if (line.find("=========================================================") != std::string::npos) break;
            }
        }
        else if (choice == 4) {
            send_line(sock, "STATUS");
            std::string status_msg;
            recv_line(sock, status_msg);
            std::cout << "\n[MY STATUS]: " << status_msg << "\n";
        }
        else if (choice == 5) {
            send_line(sock, "EXIT");
            std::string exit_msg;
            recv_line(sock, exit_msg);
            std::cout << "[SERVER]: " << exit_msg << "\n";
            break;
        }
        else {
            std::cout << "Invalid selection. Please choose 1 to 5.\n";
        }
    }

    closesocket(sock);
    cleanup_platform();
    std::cout << "Disconnected from Contest Server.\n";
    return 0;
}
