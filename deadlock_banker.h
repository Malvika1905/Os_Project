#ifndef DEADLOCK_BANKER_H
#define DEADLOCK_BANKER_H

/**
 * ============================================================================
 * MODULE 4: DEADLOCK AVOIDANCE & SECONDARY STORAGE LOGGING ENGINE
 * Author: Malvika Saxena (Enrollment ID: 992401030325)
 * Course: Operating Systems Lab (25B16CS211) - JIIT
 * Supervisor: Asst. Prof. Satya Prakash Patel
 *
 * Covers OS Syllabus:
 *  - Module 4: System Resource Model, Deadlock Characterization,
 *              Deadlock Avoidance via Banker's Algorithm (Safe State Check),
 *              Resource Request Algorithm, and Safe Sequence Discovery.
 *  - Module 6: Secondary Storage Management & Disk Block Persistence.
 *  - Module 7: Security Authentication & Protected Access.
 * ============================================================================
 */

#include <string>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
    typedef CRITICAL_SECTION banker_mutex_t;
#else
    #include <pthread.h>
    typedef pthread_mutex_t banker_mutex_t;
#endif

// Represents resources: Resource 0 = CPU Cores, Resource 1 = Memory Blocks, Resource 2 = Sandbox Tokens
struct ResourceVector {
    int cpu;
    int mem;
    int sandbox;
};

// Allocation entry for a student process / evaluation thread
struct ProcessResourceAllocation {
    std::string process_name;
    ResourceVector allocated;
    ResourceVector max_claim;
    ResourceVector need;
};

class DeadlockBankerEngine {
public:
    DeadlockBankerEngine(int total_cpu, int total_mem, int total_sandbox);
    ~DeadlockBankerEngine();

    // Banker's Algorithm: Resource Request & Safe State Evaluation
    bool request_resources(
        const std::string& process_name, 
        const ResourceVector& request, 
        const ResourceVector& max_claim,
        std::string& status_explanation
    );
    void release_resources(const std::string& process_name, const ResourceVector& released);

    // Safety Algorithm
    bool is_system_in_safe_state(std::vector<std::string>& safe_sequence_out);

    // Resource Queries
    ResourceVector get_available_resources();
    std::string generate_resource_matrix_table();

    // Module 6: Secondary Storage & Persistent Disk Logging
    void append_persistent_log(const std::string& subsystem, const std::string& message);
    std::string read_tail_persistent_log(int lines_count);

    // Module 7: Security & Protection
    bool authenticate_session_token(const std::string& token, std::string& out_user);
    std::string issue_session_token(const std::string& username);

private:
    ResourceVector m_total_system;
    ResourceVector m_available;
    std::vector<ProcessResourceAllocation> m_allocations;

    banker_mutex_t m_banker_mutex;
    banker_mutex_t m_disk_mutex;

    void init_mutexes();
    void destroy_mutexes();
    void lock_banker();
    void unlock_banker();
    void lock_disk();
    void unlock_disk();

    int find_process_index(const std::string& process_name);
};

#endif // DEADLOCK_BANKER_H
