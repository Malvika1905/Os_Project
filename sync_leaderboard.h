#ifndef SYNC_LEADERBOARD_H
#define SYNC_LEADERBOARD_H

/**
 * ============================================================================
 * MODULE 2: PROCESS SYNCHRONIZATION & LEADERBOARD STATE ENGINE
 * Author: Ayushi Tripathi (Enrollment ID: 992401030317)
 * Course: Operating Systems Lab (25B16CS211) - JIIT
 * Supervisor: Asst. Prof. Satya Prakash Patel
 *
 * Covers OS Syllabus:
 *  - Module 3: Critical Section Problem, Mutual Exclusion (Mutex Locks),
 *              Race Condition Prevention, Atomic State Updates, and
 *              Contest Score Integrity.
 * ============================================================================
 */

#include <string>
#include <vector>
#include <map>

#ifdef _WIN32
    #include <windows.h>
    typedef CRITICAL_SECTION sync_mutex_t;
#else
    #include <pthread.h>
    typedef pthread_mutex_t sync_mutex_t;
#endif

// Record for a single participant
struct ParticipantRecord {
    std::string username;
    int total_score;
    int accepted_submissions;
    int total_submissions;
    long long last_submission_epoch;
    std::vector<int> solved_problem_ids;
};

// Summary metrics of the contest state
struct ContestMetrics {
    int total_participants;
    int total_submissions;
    int total_accepted;
    double average_score;
    std::string top_scorer;
    int highest_score;
};

class SyncLeaderboardEngine {
public:
    SyncLeaderboardEngine();
    ~SyncLeaderboardEngine();

    // Participant lifecycle & registration
    bool register_or_get_participant(const std::string& username, ParticipantRecord& record_out);
    bool is_participant_registered(const std::string& username);

    // Critical Section: Score Updates (Race Condition Prevention)
    bool apply_submission_result(
        const std::string& username, 
        int problem_id, 
        int points, 
        bool is_accepted, 
        long long timestamp,
        int& updated_score_out,
        bool& first_time_solve_out
    );

    // Queries (Protected by Readers/Critical Section)
    bool get_participant_stats(const std::string& username, ParticipantRecord& record_out);
    std::string generate_formatted_leaderboard();
    std::string generate_json_leaderboard();
    ContestMetrics compute_metrics();

    // Lock diagnostics for Viva demonstration
    long long get_lock_acquisition_count();
    long long get_contention_events();

private:
    std::vector<ParticipantRecord> m_participants;
    sync_mutex_t m_leaderboard_mutex;

    // Concurrency performance metrics
    long long m_lock_acquisitions;
    long long m_contention_events;

    void init_mutex();
    void destroy_mutex();
    void enter_critical_section();
    void leave_critical_section();

    int find_participant_index_unsafe(const std::string& username);
};

#endif // SYNC_LEADERBOARD_H
