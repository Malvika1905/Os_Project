/**
 * ============================================================================
 * MODULE 1 IMPLEMENTATION: NETWORKING & CONNECTION MANAGEMENT
 * Author: Akriti Singh (Enrollment ID: 992401030302)
 * Course: Operating Systems Lab (25B16CS211) - JIIT
 * Supervisor: Asst. Prof. Satya Prakash Patel
 *
 * Description:
 *   Implements the core networking subsystem for the Real-Time Coding Contest
 *   Server. Creates the TCP master listening socket, manages cross-platform
 *   initialization (Windows Winsock & Linux POSIX sockets), and handles
 *   concurrent incoming client connections by creating dedicated worker
 *   threads per participant. Enforces clean socket session lifecycle, line-based
 *   message framing, session tracking, and broadcast capabilities.
 * ============================================================================
 */

#include "network_engine.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <ctime>

// Wrapper argument struct passed to thread entry point
struct ThreadArgWrapper {
    NetworkEngine* engine;
    ClientSession* session;
    CommandHandlerFunc handler;
};

// Cross-platform thread worker function
#ifdef _WIN32
static DWORD WINAPI internal_worker_th(LPVOID lpParam) {
#else
static void* internal_worker_th(void* lpParam) {
#endif
    ThreadArgWrapper* wrapper = (ThreadArgWrapper*)lpParam;
    NetworkEngine* engine = wrapper->engine;
    ClientSession* session = wrapper->session;
    CommandHandlerFunc handler = wrapper->handler;
    delete wrapper;

    std::string incoming_line;
    // Initial banner sent to connected client
    NetworkEngine::send_string(session->client_socket, 
        "200 Welcome to Real-Time Coding Contest Server (OS Lab 25B16CS211)");

    // Connection receive loop
    while (session->is_active) {
        bool ok = NetworkEngine::receive_line(session->client_socket, incoming_line);
        if (!ok) {
            // Client closed connection or network error occurred
            break;
        }

        if (incoming_line.empty()) {
            continue;
        }

        // Process request through assigned command handler callback
        if (handler != nullptr) {
            handler(session, incoming_line);
        }

        // If client issued EXIT/QUIT command, terminate thread loop
        if (incoming_line == "EXIT" || incoming_line == "QUIT") {
            session->is_active = false;
            break;
        }
    }

    // Unregister session and close socket
    engine->unregister_session(session);
    NetworkEngine::close_client_socket(session->client_socket);
    delete session;

    return 0;
}

// ----------------------------------------------------------------------------
// CONSTRUCTOR & DESTRUCTOR
// ----------------------------------------------------------------------------
NetworkEngine::NetworkEngine(int port, int max_backlog)
    : m_port(port),
      m_max_backlog(max_backlog),
      m_is_running(false),
      m_master_socket(INVALID_SOCKET),
      m_command_handler(nullptr) {
    init_mutex();
}

NetworkEngine::~NetworkEngine() {
    stop_server();
#ifdef _WIN32
    DeleteCriticalSection(&m_sessions_mutex);
#else
    pthread_mutex_destroy(&m_sessions_mutex);
#endif
}

// ----------------------------------------------------------------------------
// MUTEX SYNCHRONIZATION HELPERS (Session List Critical Section)
// ----------------------------------------------------------------------------
void NetworkEngine::init_mutex() {
#ifdef _WIN32
    InitializeCriticalSection(&m_sessions_mutex);
#else
    pthread_mutex_init(&m_sessions_mutex, NULL);
#endif
}

void NetworkEngine::lock_sessions() {
#ifdef _WIN32
    EnterCriticalSection(&m_sessions_mutex);
#else
    pthread_mutex_lock(&m_sessions_mutex);
#endif
}

void NetworkEngine::unlock_sessions() {
#ifdef _WIN32
    LeaveCriticalSection(&m_sessions_mutex);
#else
    pthread_mutex_unlock(&m_sessions_mutex);
#endif
}

// ----------------------------------------------------------------------------
// PLATFORM INITIALIZATION (Module 2: OS Abstraction)
// ----------------------------------------------------------------------------
bool NetworkEngine::initialize_platform() {
#ifdef _WIN32
    WSADATA wsa_data;
    int res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (res != 0) {
        std::cerr << "[NetworkEngine] WSAStartup failed with error code: " << res << "\n";
        return false;
    }
    std::cout << "[NetworkEngine] Winsock 2.2 subsystem successfully initialized.\n";
#else
    std::cout << "[NetworkEngine] POSIX Networking subsystem ready.\n";
#endif
    return true;
}

// ----------------------------------------------------------------------------
// MASTER LISTENING SOCKET INITIALIZATION
// ----------------------------------------------------------------------------
bool NetworkEngine::start_server() {
    // Step 1: Create master TCP stream socket (AF_INET, SOCK_STREAM)
    m_master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_master_socket == INVALID_SOCKET) {
        std::cerr << "[NetworkEngine] Master socket creation failed!\n";
        return false;
    }

    // Step 2: Configure SO_REUSEADDR socket option to prevent address reuse delays
    int opt = 1;
#ifdef _WIN32
    setsockopt(m_master_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(m_master_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    // Step 3: Bind socket to local address and configured port
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(m_port);

    if (bind(m_master_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "[NetworkEngine] Socket bind failed on port: " << m_port << "\n";
        close_client_socket(m_master_socket);
        m_master_socket = INVALID_SOCKET;
        return false;
    }

    // Step 4: Put socket into listening state with configured backlog queue
    if (listen(m_master_socket, m_max_backlog) == SOCKET_ERROR) {
        std::cerr << "[NetworkEngine] Socket listen failed with backlog: " << m_max_backlog << "\n";
        close_client_socket(m_master_socket);
        m_master_socket = INVALID_SOCKET;
        return false;
    }

    m_is_running = true;
    std::cout << "[NetworkEngine] Master socket listening on port " << m_port 
              << " (Max Connection Backlog: " << m_max_backlog << ")\n";
    return true;
}

void NetworkEngine::set_command_handler(CommandHandlerFunc handler) {
    m_command_handler = handler;
}

// ----------------------------------------------------------------------------
// ACCEPT LOOP: SPAWNS THREAD PER PARTICIPANT
// ----------------------------------------------------------------------------
void NetworkEngine::run_accept_loop() {
    if (!m_is_running || m_master_socket == INVALID_SOCKET) {
        std::cerr << "[NetworkEngine] Server must be started before running accept loop.\n";
        return;
    }

    std::cout << "[NetworkEngine] Entering concurrent client accept loop...\n";

    while (m_is_running) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // Blocking call waiting for incoming participant TCP connection
        socket_handle_t client_sock = accept(m_master_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock == INVALID_SOCKET) {
            if (!m_is_running) {
                // Server shutdown initiated
                break;
            }
            std::cerr << "[NetworkEngine] Warning: accept() encountered an invalid socket.\n";
            continue;
        }

        // Extract client IP and port for tracking
        char* ip_ptr = inet_ntoa(client_addr.sin_addr);
        std::string client_ip = (ip_ptr != nullptr) ? std::string(ip_ptr) : "127.0.0.1";
        int client_port = ntohs(client_addr.sin_port);

        // Allocate a new session descriptor
        ClientSession* session = new ClientSession();
        session->client_socket = client_sock;
        session->ip_address = client_ip;
        session->port = client_port;
        session->authenticated_user = "Anonymous";
        session->is_active = true;
        session->connection_timestamp = (long long)time(nullptr);

        // Register session in thread-safe active sessions vector
        register_session(session);

        std::cout << "[NetworkEngine] Incoming connection accepted from " << client_ip 
                  << ":" << client_port << ". Spawning dedicated worker thread...\n";

        // Package arguments for thread entry function
        ThreadArgWrapper* wrapper = new ThreadArgWrapper();
        wrapper->engine = this;
        wrapper->session = session;
        wrapper->handler = m_command_handler;

        // Create dedicated worker thread (Module 3: Threading Model)
#ifdef _WIN32
        HANDLE th = CreateThread(NULL, 0, internal_worker_th, wrapper, 0, NULL);
        if (th == NULL) {
            std::cerr << "[NetworkEngine] Failed to create thread for client!\n";
            unregister_session(session);
            close_client_socket(client_sock);
            delete session;
            delete wrapper;
        } else {
            CloseHandle(th); // Detach thread so resources auto-reclaim on exit
        }
#else
        pthread_t pt;
        int th_res = pthread_create(&pt, NULL, internal_worker_th, wrapper);
        if (th_res != 0) {
            std::cerr << "[NetworkEngine] pthread_create failed with error: " << th_res << "\n";
            unregister_session(session);
            close_client_socket(client_sock);
            delete session;
            delete wrapper;
        } else {
            pthread_detach(pt); // Detach thread for automatic reclamation
        }
#endif
    }

    std::cout << "[NetworkEngine] Accept loop terminated.\n";
}

// ----------------------------------------------------------------------------
// SERVER SHUTDOWN & CLEANUP
// ----------------------------------------------------------------------------
void NetworkEngine::stop_server() {
    if (!m_is_running) return;
    m_is_running = false;

    // Close master listening socket to unblock accept()
    if (m_master_socket != INVALID_SOCKET) {
        close_client_socket(m_master_socket);
        m_master_socket = INVALID_SOCKET;
    }

    // Close and disconnect all active client sessions
    lock_sessions();
    for (size_t i = 0; i < m_active_sessions.size(); ++i) {
        if (m_active_sessions[i] != nullptr) {
            m_active_sessions[i]->is_active = false;
            send_string(m_active_sessions[i]->client_socket, "503 Server shutting down. Goodbye!");
            close_client_socket(m_active_sessions[i]->client_socket);
        }
    }
    m_active_sessions.clear();
    unlock_sessions();

#ifdef _WIN32
    WSACleanup();
#endif
    std::cout << "[NetworkEngine] Server stopped and network resources cleaned up.\n";
}

// ----------------------------------------------------------------------------
// ACTIVE SESSION REGISTRY (Thread-Safe Critical Section)
// ----------------------------------------------------------------------------
void NetworkEngine::register_session(ClientSession* session) {
    lock_sessions();
    m_active_sessions.push_back(session);
    unlock_sessions();
}

void NetworkEngine::unregister_session(ClientSession* session) {
    lock_sessions();
    auto it = std::find(m_active_sessions.begin(), m_active_sessions.end(), session);
    if (it != m_active_sessions.end()) {
        m_active_sessions.erase(it);
    }
    unlock_sessions();
}

int NetworkEngine::get_active_connection_count() {
    lock_sessions();
    int count = (int)m_active_sessions.size();
    unlock_sessions();
    return count;
}

void NetworkEngine::broadcast_message(const std::string& broadcast_text) {
    lock_sessions();
    for (size_t i = 0; i < m_active_sessions.size(); ++i) {
        if (m_active_sessions[i] != nullptr && m_active_sessions[i]->is_active) {
            send_string(m_active_sessions[i]->client_socket, "[BROADCAST] " + broadcast_text);
        }
    }
    unlock_sessions();
}

// ----------------------------------------------------------------------------
// STATIC SOCKET UTILITIES (Line-based Framing)
// ----------------------------------------------------------------------------
bool NetworkEngine::send_string(socket_handle_t sock, const std::string& data) {
    std::string framed_data = data + "\n";
    int total_bytes = (int)framed_data.length();
    int bytes_sent = 0;

    while (bytes_sent < total_bytes) {
        int res = send(sock, framed_data.c_str() + bytes_sent, total_bytes - bytes_sent, 0);
        if (res == SOCKET_ERROR || res <= 0) {
            return false;
        }
        bytes_sent += res;
    }
    return true;
}

bool NetworkEngine::receive_line(socket_handle_t sock, std::string& line_out) {
    line_out.clear();
    char buffer_char;

    while (true) {
        int res = recv(sock, &buffer_char, 1, 0);
        if (res <= 0) {
            // Socket closed or error
            return false;
        }
        if (buffer_char == '\n') {
            break; // Complete line received
        }
        if (buffer_char != '\r') {
            line_out += buffer_char;
        }
    }
    return true;
}

void NetworkEngine::close_client_socket(socket_handle_t sock) {
    if (sock != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }
}
