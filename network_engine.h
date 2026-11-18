#ifndef NETWORK_ENGINE_H
#define NETWORK_ENGINE_H

/**
 * ============================================================================
 * MODULE 1: NETWORKING & CONNECTION MANAGEMENT
 * Author: Akriti Singh (Enrollment ID: 992401030302)
 * Course: Operating Systems Lab (25B16CS211) - JIIT
 * Supervisor: Asst. Prof. Satya Prakash Patel
 *
 * Covers OS Syllabus:
 *  - Module 1: Historical Context & Time-Sharing Multi-User Systems
 *  - Module 2: OS Structure, Client-Server Architecture & Socket IPC
 *  - Module 3: Thread Creation & Worker Lifecycle Management
 * ============================================================================
 */

#include <string>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    typedef SOCKET socket_handle_t;
    typedef HANDLE os_thread_t;
    typedef CRITICAL_SECTION os_mutex_t;
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <pthread.h>
    typedef int socket_handle_t;
    typedef pthread_t os_thread_t;
    typedef pthread_mutex_t os_mutex_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

// Client connection session descriptor
struct ClientSession {
    socket_handle_t client_socket;
    std::string ip_address;
    int port;
    std::string authenticated_user;
    bool is_active;
    long long connection_timestamp;
};

// Callback type for handling client commands
typedef void (*CommandHandlerFunc)(ClientSession* session, const std::string& request_line);

class NetworkEngine {
public:
    NetworkEngine(int port, int max_backlog);
    ~NetworkEngine();

    bool initialize_platform();
    bool start_server();
    void set_command_handler(CommandHandlerFunc handler);
    void run_accept_loop();
    void stop_server();

    // Utility methods for socket I/O
    static bool send_string(socket_handle_t sock, const std::string& data);
    static bool receive_line(socket_handle_t sock, std::string& line_out);
    static void close_client_socket(socket_handle_t sock);

    // Active session tracking
    int get_active_connection_count();
    void register_session(ClientSession* session);
    void unregister_session(ClientSession* session);
    void broadcast_message(const std::string& broadcast_text);

private:
    int m_port;
    int m_max_backlog;
    bool m_is_running;
    socket_handle_t m_master_socket;
    CommandHandlerFunc m_command_handler;

    std::vector<ClientSession*> m_active_sessions;
    os_mutex_t m_sessions_mutex;

    void init_mutex();
    void lock_sessions();
    void unlock_sessions();
};

#endif // NETWORK_ENGINE_H
