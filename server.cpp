/**
 * ============================================================================
 * Project Title : Real-Time Coding Contest Server
 * Course        : Operating Systems Lab (25B16CS211) - JIIT
 * Supervisor    : Asst. Prof. Satya Prakash Patel
 *
 * TEAM CONTRIBUTORS & MODULE OWNERSHIP:
 *  - Akriti Singh     (992401030302): Module 1 - NetworkEngine (Sockets & Threads)
 *  - Ayushi Tripathi  (992401030317): Module 2 - SyncLeaderboardEngine (Mutex & State)
 *  - Avni Bansal      (992401030319): Module 3 - CPUSchedulerEngine (Semaphores & FCFS)
 *  - Malvika Saxena   (992401030325): Module 4 - DeadlockBankerEngine (Banker's & Disk)
 * ============================================================================
 */

#include "network_engine.h"
#include "sync_leaderboard.h"
#include "cpu_scheduler.h"
#include "deadlock_banker.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <ctime>

#ifdef _WIN32
    #include <windows.h>
    inline void os_sleep(int ms) { Sleep(ms); }
#else
    #include <unistd.h>
    inline void os_sleep(int ms) { usleep(ms * 1000); }
#endif

// Global Subsystem Instances
static NetworkEngine*         g_network_engine   = nullptr;
static SyncLeaderboardEngine* g_sync_leaderboard = nullptr;
static CPUSchedulerEngine*    g_cpu_scheduler    = nullptr;
static DeadlockBankerEngine*  g_deadlock_banker  = nullptr;

// Problems Database
struct ContestProblemDef {
    int id;
    std::string title;
    std::string description;
    int points;
    ResourceVector claim;
};

static std::vector<ContestProblemDef> g_problems_list;

void setup_contest_problems() {
    g_problems_list.push_back({
        1,
        "Reverse Array (Easy)",
        "Given an array of integers, output the elements in reverse order.",
        20,
        {1, 1, 1} // 1 CPU, 1 Mem, 1 Sandbox
    });
    g_problems_list.push_back({
        2,
        "Producer-Consumer Bounded Buffer (Medium)",
        "Synchronize concurrent producer and consumer threads using semaphores.",
        30,
        {2, 1, 1} // 2 CPU, 1 Mem, 1 Sandbox
    });
    g_problems_list.push_back({
        3,
        "Banker's Algorithm Deadlock Detection (Hard)",
        "Given Allocation, Max, and Available matrices, verify safe state.",
        50,
        {2, 2, 1} // 2 CPU, 2 Mem, 1 Sandbox
    });
}

// ----------------------------------------------------------------------------
// DISPATCH & COMMAND HANDLER (Invoked inside per-client worker threads)
// ----------------------------------------------------------------------------
void on_client_request(ClientSession* session, const std::string& request_line) {
    std::istringstream iss(request_line);
    std::string cmd;
    iss >> cmd;

    // Command: LOGIN <username>
    if (cmd == "LOGIN") {
        std::string username;
        iss >> username;
        if (username.empty()) {
            NetworkEngine::send_string(session->client_socket, "400 ERR Missing username");
            return;
        }

        session->authenticated_user = username;
        ParticipantRecord rec;
        g_sync_leaderboard->register_or_get_participant(username, rec);

        std::string token = g_deadlock_banker->issue_session_token(username);
        g_deadlock_banker->append_persistent_log("AUTH", "Authenticated user: [" + username + "] with token: " + token);

        std::cout << "[AUTH] Participant logged in: [" << username << "]\n";
        NetworkEngine::send_string(session->client_socket, "200 OK Logged in as " + username + " (Token: " + token + ")");
    }
    // Access Control (Module 7: Security)
    else if (session->authenticated_user == "Anonymous" && cmd != "EXIT" && cmd != "QUIT") {
        NetworkEngine::send_string(session->client_socket, "401 ERR Please LOGIN first");
    }
    // Command: PROBLEMS
    else if (cmd == "PROBLEMS") {
        std::ostringstream ss;
        ss << "\n--- AVAILABLE CONTEST PROBLEMS ---\n";
        for (const auto& p : g_problems_list) {
            ss << "[" << p.id << "] " << p.title << " (" << p.points << " pts)\n"
               << "    Description   : " << p.description << "\n"
               << "    Resource Claim: CPU=" << p.claim.cpu << ", MEM=" << p.claim.mem << ", SANDBOX=" << p.claim.sandbox << "\n";
        }
        ss << "--- END OF PROBLEMS ---";
        NetworkEngine::send_string(session->client_socket, ss.str());
    }
    // Command: SUBMIT <prob_id> <code>
    else if (cmd == "SUBMIT") {
        int prob_id = 0;
        iss >> prob_id;
        std::string code;
        std::getline(iss, code);
        size_t first_char = code.find_first_not_of(" \t");
        if (first_char != std::string::npos) code = code.substr(first_char);

        ContestProblemDef* target_prob = nullptr;
        for (auto& p : g_problems_list) {
            if (p.id == prob_id) { target_prob = &p; break; }
        }

        if (!target_prob) {
            NetworkEngine::send_string(session->client_socket, "404 ERR Problem ID not found");
            return;
        }

        // 1. Enqueue task in CPU Scheduler (Module 3)
        int task_id = g_cpu_scheduler->enqueue_task(
            session->authenticated_user, 
            prob_id, 
            code, 
            target_prob->points, 
            1000
        );

        g_deadlock_banker->append_persistent_log("SCHEDULER", 
            "Enqueued Task #" + std::to_string(task_id) + " for [" + session->authenticated_user + "]");

        NetworkEngine::send_string(session->client_socket, 
            "100 Queued submission #" + std::to_string(task_id) + ". Waiting for evaluator slot...");

        // 2. Throttling via Counting Semaphore (Module 3)
        std::cout << "[SEMAPHORE] [" << session->authenticated_user << "] waiting for evaluator slot...\n";
        g_cpu_scheduler->acquire_evaluator_slot();
        std::cout << "[SEMAPHORE] Slot acquired by [" << session->authenticated_user << "].\n";

        // 3. Deadlock Avoidance via Banker's Algorithm (Module 4)
        std::string banker_msg;
        ResourceVector requested = target_prob->claim;
        bool safe = false;

        while (!safe) {
            safe = g_deadlock_banker->request_resources(
                session->authenticated_user, 
                requested, 
                target_prob->claim, 
                banker_msg
            );

            if (!safe) {
                std::cout << "[DEADLOCK-BANKER] " << banker_msg << "\n";
                os_sleep(200); // Back off and retry when safe state emerges
            }
        }

        std::cout << "[DEADLOCK-BANKER] Resources allocated to [" << session->authenticated_user << "]: " << banker_msg << "\n";
        g_deadlock_banker->append_persistent_log("DEADLOCK-SAFE", 
            "Resources allocated to [" + session->authenticated_user + "]");

        // 4. Simulate test execution time
        os_sleep(1000);
        bool accepted = (code.length() > 3);
        std::string verdict = accepted ? "ACCEPTED" : "WRONG_ANSWER";

        // 5. Critical Section: Update Synchronized Leaderboard (Module 3: Mutex)
        int final_score = 0;
        bool is_first_solve = false;
        long long now_epoch = (long long)time(nullptr);

        g_sync_leaderboard->apply_submission_result(
            session->authenticated_user,
            prob_id,
            target_prob->points,
            accepted,
            now_epoch,
            final_score,
            is_first_solve
        );

        std::cout << "[CRITICAL-SEC] Score updated for [" << session->authenticated_user 
                  << "] -> Total Score: " << final_score << "\n";
        g_deadlock_banker->append_persistent_log("SCORE", 
            "User [" + session->authenticated_user + "] updated score to " + std::to_string(final_score));

        // 6. Release resources and Semaphore slot
        g_deadlock_banker->release_resources(session->authenticated_user, requested);
        g_cpu_scheduler->release_evaluator_slot();
        std::cout << "[SEMAPHORE] Evaluator slot released by [" << session->authenticated_user << "].\n";

        // 7. Send result back to client
        std::string reply = "200 RESULT: " + verdict + " (+" + 
            std::to_string(accepted && is_first_solve ? target_prob->points : 0) + 
            " pts) | Your Total Score: " + std::to_string(final_score);
        NetworkEngine::send_string(session->client_socket, reply);
    }
    // Command: LEADERBOARD
    else if (cmd == "LEADERBOARD") {
        std::string board = g_sync_leaderboard->generate_formatted_leaderboard();
        board += "--- END OF LEADERBOARD ---";
        NetworkEngine::send_string(session->client_socket, board);
    }
    // Command: STATUS
    else if (cmd == "STATUS") {
        ParticipantRecord rec;
        if (g_sync_leaderboard->get_participant_stats(session->authenticated_user, rec)) {
            std::ostringstream ss;
            ss << "Participant: " << rec.username 
               << " | Score: " << rec.total_score << " pts"
               << " | Solved: " << rec.solved_problem_ids.size() << " / " << g_problems_list.size()
               << " | Submissions: " << rec.total_submissions;
            if (!rec.solved_problem_ids.empty()) {
                ss << " | Solved Problem IDs: [";
                for (size_t i = 0; i < rec.solved_problem_ids.size(); ++i) {
                    ss << rec.solved_problem_ids[i] << (i + 1 < rec.solved_problem_ids.size() ? ", " : "");
                }
                ss << "]";
            }
            NetworkEngine::send_string(session->client_socket, ss.str());
        } else {
            NetworkEngine::send_string(session->client_socket, "404 ERR Participant record not found");
        }
    }
    // Command: QUEUE
    else if (cmd == "QUEUE") {
        std::string q_report = g_cpu_scheduler->get_queue_status_report();
        NetworkEngine::send_string(session->client_socket, q_report);
    }
    // Command: RESOURCES
    else if (cmd == "RESOURCES") {
        std::string matrix = g_deadlock_banker->generate_resource_matrix_table();
        NetworkEngine::send_string(session->client_socket, matrix);
    }
    // Command: EXIT
    else if (cmd == "EXIT" || cmd == "QUIT") {
        NetworkEngine::send_string(session->client_socket, "200 Goodbye!");
        session->is_active = false;
    }
    else {
        NetworkEngine::send_string(session->client_socket, "400 ERR Unknown command. Available: LOGIN, PROBLEMS, SUBMIT, LEADERBOARD, STATUS, QUEUE, RESOURCES, EXIT");
    }
}

// ----------------------------------------------------------------------------
// MAIN SERVER ENTRY POINT
// ----------------------------------------------------------------------------
int main() {
    std::cout << "=========================================================================\n";
    std::cout << "          REAL-TIME CODING CONTEST SERVER (OS LAB 25B16CS211)            \n";
    std::cout << "  Akriti Singh (302) | Ayushi Tripathi (317) | Avni (319) | Malvika (325) \n";
    std::cout << "  Department of CSE & IT, Jaypee Institute of Information Technology     \n";
    std::cout << "  Submitted To: Asst. Prof. Satya Prakash Patel                          \n";
    std::cout << "=========================================================================\n";

    setup_contest_problems();

    // Initialize all four OS Subsystem Engines
    g_network_engine   = new NetworkEngine(8080, 20);
    g_sync_leaderboard = new SyncLeaderboardEngine();
    g_cpu_scheduler    = new CPUSchedulerEngine(2, POLICY_FCFS); // 2 evaluator semaphores, FCFS policy
    g_deadlock_banker  = new DeadlockBankerEngine(4, 4, 2);      // 4 CPU, 4 Mem, 2 Sandbox capacity

    if (!g_network_engine->initialize_platform()) {
        std::cerr << "Fatal: Failed to initialize network platform.\n";
        return 1;
    }

    g_network_engine->set_command_handler(on_client_request);

    if (!g_network_engine->start_server()) {
        std::cerr << "Fatal: Failed to start server listening on port 8080.\n";
        return 1;
    }

    g_deadlock_banker->append_persistent_log("SYS-BOOT", "Contest server booted successfully on port 8080.");
    std::cout << "[SYSTEM] All 4 OS Engines Initialized. Waiting for student connections...\n\n";

    // Enter accept loop (blocking)
    g_network_engine->run_accept_loop();

    // Clean up
    delete g_network_engine;
    delete g_sync_leaderboard;
    delete g_cpu_scheduler;
    delete g_deadlock_banker;

    return 0;
}
