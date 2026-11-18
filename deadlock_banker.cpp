/**
 * ============================================================================
 * MODULE 4 IMPLEMENTATION: DEADLOCK AVOIDANCE & SECONDARY STORAGE LOGGING
 * Author: Malvika Saxena (Enrollment ID: 992401030325)
 * Course: Operating Systems Lab (25B16CS211) - JIIT
 * Supervisor: Asst. Prof. Satya Prakash Patel
 *
 * Description:
 *   Implements Deadlock Avoidance through Edsger Dijkstra's classic Banker's
 *   Algorithm, alongside secondary storage persistence and security controls.
 *   Evaluates whether granting system resources (CPU Cores, Memory Blocks,
 *   Sandbox Tokens) to incoming compilation jobs leaves the system in a Safe State.
 *   Maintains Allocation, Max, Available, and Need matrices, computes full
 *   safety sequences, logs all operations chronologically to non-volatile disk
 *   storage (contest_log.txt), and verifies access permissions.
 * ============================================================================
 */

#include "deadlock_banker.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

// ----------------------------------------------------------------------------
// CONSTRUCTOR & DESTRUCTOR
// ----------------------------------------------------------------------------
DeadlockBankerEngine::DeadlockBankerEngine(int total_cpu, int total_mem, int total_sandbox) {
    m_total_system.cpu = total_cpu;
    m_total_system.mem = total_mem;
    m_total_system.sandbox = total_sandbox;

    m_available = m_total_system;

    init_mutexes();
}

DeadlockBankerEngine::~DeadlockBankerEngine() {
    destroy_mutexes();
}

// ----------------------------------------------------------------------------
// MUTEX INITIALIZATION (Critical section for matrices & disk file)
// ----------------------------------------------------------------------------
void DeadlockBankerEngine::init_mutexes() {
#ifdef _WIN32
    InitializeCriticalSection(&m_banker_mutex);
    InitializeCriticalSection(&m_disk_mutex);
#else
    pthread_mutex_init(&m_banker_mutex, NULL);
    pthread_mutex_init(&m_disk_mutex, NULL);
#endif
}

void DeadlockBankerEngine::destroy_mutexes() {
#ifdef _WIN32
    DeleteCriticalSection(&m_banker_mutex);
    DeleteCriticalSection(&m_disk_mutex);
#else
    pthread_mutex_destroy(&m_banker_mutex);
    pthread_mutex_destroy(&m_disk_mutex);
#endif
}

void DeadlockBankerEngine::lock_banker() {
#ifdef _WIN32
    EnterCriticalSection(&m_banker_mutex);
#else
    pthread_mutex_lock(&m_banker_mutex);
#endif
}

void DeadlockBankerEngine::unlock_banker() {
#ifdef _WIN32
    LeaveCriticalSection(&m_banker_mutex);
#else
    pthread_mutex_unlock(&m_banker_mutex);
#endif
}

void DeadlockBankerEngine::lock_disk() {
#ifdef _WIN32
    EnterCriticalSection(&m_disk_mutex);
#else
    pthread_mutex_lock(&m_disk_mutex);
#endif
}

void DeadlockBankerEngine::unlock_disk() {
#ifdef _WIN32
    LeaveCriticalSection(&m_disk_mutex);
#else
    pthread_mutex_unlock(&m_disk_mutex);
#endif
}

int DeadlockBankerEngine::find_process_index(const std::string& process_name) {
    for (size_t i = 0; i < m_allocations.size(); ++i) {
        if (m_allocations[i].process_name == process_name) {
            return (int)i;
        }
    }
    return -1;
}

// ----------------------------------------------------------------------------
// MODULE 4: BANKER'S SAFETY ALGORITHM (Checks if a Safe Sequence exists)
// ----------------------------------------------------------------------------
bool DeadlockBankerEngine::is_system_in_safe_state(std::vector<std::string>& safe_sequence_out) {
    safe_sequence_out.clear();

    // 1. Let Work be an integer vector initialized to Available
    ResourceVector work = m_available;

    // 2. Let Finish be a boolean array of size N initialized to false
    int n = (int)m_allocations.size();
    if (n == 0) return true;

    std::vector<bool> finish(n, false);

    // 3. Find an index i such that:
    //    a. Finish[i] == false
    //    b. Need[i] <= Work
    bool found_candidate = true;
    while (found_candidate) {
        found_candidate = false;

        for (int i = 0; i < n; ++i) {
            if (!finish[i]) {
                const auto& p = m_allocations[i];
                // Check Need <= Work for all resource types
                if (p.need.cpu <= work.cpu &&
                    p.need.mem <= work.mem &&
                    p.need.sandbox <= work.sandbox) {
                    
                    // Work = Work + Allocation[i]
                    work.cpu += p.allocated.cpu;
                    work.mem += p.allocated.mem;
                    work.sandbox += p.allocated.sandbox;

                    finish[i] = true;
                    safe_sequence_out.push_back(p.process_name);
                    found_candidate = true;
                }
            }
        }
    }

    // 4. If Finish[i] == true for all i, then system is in a SAFE state
    for (int i = 0; i < n; ++i) {
        if (!finish[i]) {
            return false; // UNSAFE state - Potential Deadlock!
        }
    }
    return true;
}

// ----------------------------------------------------------------------------
// MODULE 4: RESOURCE-REQUEST ALGORITHM (Banker's Algorithm Core)
// ----------------------------------------------------------------------------
bool DeadlockBankerEngine::request_resources(
    const std::string& process_name, 
    const ResourceVector& request, 
    const ResourceVector& max_claim,
    std::string& status_explanation
) {
    lock_banker();

    int idx = find_process_index(process_name);
    if (idx == -1) {
        ProcessResourceAllocation new_p;
        new_p.process_name = process_name;
        new_p.allocated = {0, 0, 0};
        new_p.max_claim = max_claim;
        new_p.need = max_claim;
        m_allocations.push_back(new_p);
        idx = (int)m_allocations.size() - 1;
    }

    auto& proc = m_allocations[idx];

    // Step 1: If Request <= Need, proceed. Else raise error.
    if (request.cpu > proc.need.cpu || 
        request.mem > proc.need.mem || 
        request.sandbox > proc.need.sandbox) {
        status_explanation = "ERR: Process exceeded its declared maximum claim!";
        unlock_banker();
        return false;
    }

    // Step 2: If Request <= Available, proceed. Else process must wait.
    if (request.cpu > m_available.cpu || 
        request.mem > m_available.mem || 
        request.sandbox > m_available.sandbox) {
        status_explanation = "WAIT: Insufficient resources available. Process must wait to prevent deadlock.";
        unlock_banker();
        return false;
    }

    // Step 3: Pretend to allocate requested resources (Hypothetical State)
    m_available.cpu -= request.cpu;
    m_available.mem -= request.mem;
    m_available.sandbox -= request.sandbox;

    proc.allocated.cpu += request.cpu;
    proc.allocated.mem += request.mem;
    proc.allocated.sandbox += request.sandbox;

    proc.need.cpu -= request.cpu;
    proc.need.mem -= request.mem;
    proc.need.sandbox -= request.sandbox;

    // Step 4: Run Safety Algorithm on hypothetical state
    std::vector<std::string> safe_seq;
    bool safe = is_system_in_safe_state(safe_seq);

    if (safe) {
        // Safe state preserved - Grant resources
        std::ostringstream ss;
        ss << "SAFE STATE PRESERVED. Safe sequence: <";
        for (size_t i = 0; i < safe_seq.size(); ++i) {
            ss << safe_seq[i] << (i + 1 < safe_seq.size() ? ", " : ">");
        }
        status_explanation = ss.str();
        unlock_banker();
        return true;
    } else {
        // Unsafe state detected - Rollback allocation!
        m_available.cpu += request.cpu;
        m_available.mem += request.mem;
        m_available.sandbox += request.sandbox;

        proc.allocated.cpu -= request.cpu;
        proc.allocated.mem -= request.mem;
        proc.allocated.sandbox -= request.sandbox;

        proc.need.cpu += request.cpu;
        proc.need.mem += request.mem;
        proc.need.sandbox += request.sandbox;

        status_explanation = "DEADLOCK AVOIDED: Allocation would lead to UNSAFE state. Request rolled back!";
        unlock_banker();
        return false;
    }
}

// ----------------------------------------------------------------------------
// RESOURCE RELEASE
// ----------------------------------------------------------------------------
void DeadlockBankerEngine::release_resources(const std::string& process_name, const ResourceVector& released) {
    lock_banker();

    int idx = find_process_index(process_name);
    if (idx != -1) {
        auto& proc = m_allocations[idx];

        proc.allocated.cpu = (std::max)(0, proc.allocated.cpu - released.cpu);
        proc.allocated.mem = (std::max)(0, proc.allocated.mem - released.mem);
        proc.allocated.sandbox = (std::max)(0, proc.allocated.sandbox - released.sandbox);

        proc.need.cpu += released.cpu;
        proc.need.mem += released.mem;
        proc.need.sandbox += released.sandbox;

        m_available.cpu = (std::min)(m_total_system.cpu, m_available.cpu + released.cpu);
        m_available.mem = (std::min)(m_total_system.mem, m_available.mem + released.mem);
        m_available.sandbox = (std::min)(m_total_system.sandbox, m_available.sandbox + released.sandbox);

        // If process holds 0 resources, clean up from active allocations
        if (proc.allocated.cpu == 0 && proc.allocated.mem == 0 && proc.allocated.sandbox == 0) {
            m_allocations.erase(m_allocations.begin() + idx);
        }
    }

    unlock_banker();
}

ResourceVector DeadlockBankerEngine::get_available_resources() {
    lock_banker();
    ResourceVector av = m_available;
    unlock_banker();
    return av;
}

// ----------------------------------------------------------------------------
// MATRIX INSPECTION FOR LAB VIVA DEMONSTRATION
// ----------------------------------------------------------------------------
std::string DeadlockBankerEngine::generate_resource_matrix_table() {
    lock_banker();

    std::ostringstream ss;
    ss << "\n=========================================================================\n";
    ss << "              BANKER'S ALGORITHM RESOURCE ALLOCATION MATRICES            \n";
    ss << "=========================================================================\n";
    ss << " Available System Resources : CPU=" << m_available.cpu 
       << ", MEM=" << m_available.mem 
       << ", SANDBOX=" << m_available.sandbox << "\n";
    ss << " Total System Capacity     : CPU=" << m_total_system.cpu 
       << ", MEM=" << m_total_system.mem 
       << ", SANDBOX=" << m_total_system.sandbox << "\n";
    ss << "-------------------------------------------------------------------------\n";
    ss << std::left 
       << std::setw(18) << "Process"
       << std::setw(18) << "Allocation(C,M,S)"
       << std::setw(18) << "Max(C,M,S)"
       << "Need(C,M,S)\n";
    ss << "-------------------------------------------------------------------------\n";

    if (m_allocations.empty()) {
        ss << "                    No processes currently holding resources.            \n";
    } else {
        for (const auto& p : m_allocations) {
            std::ostringstream a_ss, m_ss, n_ss;
            a_ss << "(" << p.allocated.cpu << "," << p.allocated.mem << "," << p.allocated.sandbox << ")";
            m_ss << "(" << p.max_claim.cpu << "," << p.max_claim.mem << "," << p.max_claim.sandbox << ")";
            n_ss << "(" << p.need.cpu << "," << p.need.mem << "," << p.need.sandbox << ")";

            ss << std::left
               << std::setw(18) << p.process_name
               << std::setw(18) << a_ss.str()
               << std::setw(18) << m_ss.str()
               << n_ss.str() << "\n";
        }
    }
    ss << "=========================================================================\n";

    unlock_banker();
    return ss.str();
}

// ----------------------------------------------------------------------------
// MODULE 6: SECONDARY STORAGE MANAGEMENT (Persistent Disk Logging)
// ----------------------------------------------------------------------------
void DeadlockBankerEngine::append_persistent_log(const std::string& subsystem, const std::string& message) {
    lock_disk();

    time_t now = time(nullptr);
    tm* ltm = localtime(&now);
    std::ostringstream time_ss;
    time_ss << std::setfill('0')
            << std::setw(2) << ltm->tm_hour << ":"
            << std::setw(2) << ltm->tm_min << ":"
            << std::setw(2) << ltm->tm_sec;

    std::string line = "[" + time_ss.str() + "] [" + subsystem + "] " + message;

    // Append to contest_log.txt on disk (Secondary Storage persistence)
    std::ofstream f("contest_log.txt", std::ios::app);
    if (f.is_open()) {
        f << line << "\n";
        f.flush();
        f.close();
    }

    unlock_disk();
}

std::string DeadlockBankerEngine::read_tail_persistent_log(int lines_count) {
    lock_disk();

    std::ifstream f("contest_log.txt");
    if (!f.is_open()) {
        unlock_disk();
        return "Log file empty or not created yet.";
    }

    std::vector<std::string> all_lines;
    std::string l;
    while (std::getline(f, l)) {
        all_lines.push_back(l);
    }
    f.close();

    int start_idx = (std::max)(0, (int)all_lines.size() - lines_count);
    std::ostringstream ss;
    for (size_t i = start_idx; i < all_lines.size(); ++i) {
        ss << all_lines[i] << "\n";
    }

    unlock_disk();
    return ss.str();
}

// ----------------------------------------------------------------------------
// MODULE 7: SECURITY & PROTECTION (Session Authentication)
// ----------------------------------------------------------------------------
std::string DeadlockBankerEngine::issue_session_token(const std::string& username) {
    std::ostringstream ss;
    ss << "AUTH_" << username << "_" << (time(nullptr) % 100000);
    return ss.str();
}

bool DeadlockBankerEngine::authenticate_session_token(const std::string& token, std::string& out_user) {
    if (token.find("AUTH_") == 0) {
        size_t first = token.find('_');
        size_t second = token.rfind('_');
        if (first != std::string::npos && second != std::string::npos && second > first) {
            out_user = token.substr(first + 1, second - first - 1);
            return true;
        }
    }
    return false;
}
