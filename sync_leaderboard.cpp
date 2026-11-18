/**
 * ============================================================================
 * MODULE 2 IMPLEMENTATION: PROCESS SYNCHRONIZATION & LEADERBOARD STATE ENGINE
 * Author: Ayushi Tripathi (Enrollment ID: 992401030317)
 * Course: Operating Systems Lab (25B16CS211) - JIIT
 * Supervisor: Asst. Prof. Satya Prakash Patel
 *
 * Description:
 *   Implements thread-safe access to the contest state, preventing race conditions
 *   and lost updates when multiple threads attempt to read and write participant
 *   scores simultaneously. Demonstrates classic Operating Systems synchronization
 *   concepts: Mutual Exclusion (Mutex Locks), Critical Section boundaries,
 *   thread safety, deterministic leaderboard sorting, and atomic state updates.
 * ============================================================================
 */

#include "sync_leaderboard.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

// ----------------------------------------------------------------------------
// CONSTRUCTOR & DESTRUCTOR
// ----------------------------------------------------------------------------
SyncLeaderboardEngine::SyncLeaderboardEngine()
    : m_lock_acquisitions(0),
      m_contention_events(0) {
    init_mutex();
}

SyncLeaderboardEngine::~SyncLeaderboardEngine() {
    destroy_mutex();
}

// ----------------------------------------------------------------------------
// MUTEX MANAGEMENT (Module 3: Synchronization Hardware & Mutex Locks)
// ----------------------------------------------------------------------------
void SyncLeaderboardEngine::init_mutex() {
#ifdef _WIN32
    InitializeCriticalSection(&m_leaderboard_mutex);
#else
    pthread_mutex_init(&m_leaderboard_mutex, NULL);
#endif
}

void SyncLeaderboardEngine::destroy_mutex() {
#ifdef _WIN32
    DeleteCriticalSection(&m_leaderboard_mutex);
#else
    pthread_mutex_destroy(&m_leaderboard_mutex);
#endif
}

void SyncLeaderboardEngine::enter_critical_section() {
#ifdef _WIN32
    // TryEnterCriticalSection helps track lock contention for OS diagnostics
    if (!TryEnterCriticalSection(&m_leaderboard_mutex)) {
        m_contention_events++;
        EnterCriticalSection(&m_leaderboard_mutex);
    }
#else
    if (pthread_mutex_trylock(&m_leaderboard_mutex) != 0) {
        m_contention_events++;
        pthread_mutex_lock(&m_leaderboard_mutex);
    }
#endif
    m_lock_acquisitions++;
}

void SyncLeaderboardEngine::leave_critical_section() {
#ifdef _WIN32
    LeaveCriticalSection(&m_leaderboard_mutex);
#else
    pthread_mutex_unlock(&m_leaderboard_mutex);
#endif
}

// ----------------------------------------------------------------------------
// INTERNAL HELPER (Assumes critical section lock is already held)
// ----------------------------------------------------------------------------
int SyncLeaderboardEngine::find_participant_index_unsafe(const std::string& username) {
    for (size_t i = 0; i < m_participants.size(); ++i) {
        if (m_participants[i].username == username) {
            return (int)i;
        }
    }
    return -1;
}

// ----------------------------------------------------------------------------
// PARTICIPANT REGISTRATION (Thread-Safe)
// ----------------------------------------------------------------------------
bool SyncLeaderboardEngine::register_or_get_participant(const std::string& username, ParticipantRecord& record_out) {
    if (username.empty()) return false;

    enter_critical_section();

    int idx = find_participant_index_unsafe(username);
    if (idx != -1) {
        record_out = m_participants[idx];
        leave_critical_section();
        return true;
    }

    // Create a new participant record
    ParticipantRecord new_rec;
    new_rec.username = username;
    new_rec.total_score = 0;
    new_rec.accepted_submissions = 0;
    new_rec.total_submissions = 0;
    new_rec.last_submission_epoch = (long long)time(nullptr);
    new_rec.solved_problem_ids.clear();

    m_participants.push_back(new_rec);
    record_out = new_rec;

    leave_critical_section();
    return true;
}

bool SyncLeaderboardEngine::is_participant_registered(const std::string& username) {
    enter_critical_section();
    bool exists = (find_participant_index_unsafe(username) != -1);
    leave_critical_section();
    return exists;
}

// ----------------------------------------------------------------------------
// CRITICAL SECTION: SCORE UPDATING (Module 3: Race Condition Prevention)
// ----------------------------------------------------------------------------
bool SyncLeaderboardEngine::apply_submission_result(
    const std::string& username, 
    int problem_id, 
    int points, 
    bool is_accepted, 
    long long timestamp,
    int& updated_score_out,
    bool& first_time_solve_out
) {
    first_time_solve_out = false;
    updated_score_out = 0;

    // CRITICAL SECTION ENTRY
    enter_critical_section();

    int idx = find_participant_index_unsafe(username);
    if (idx == -1) {
        // Auto-register if not present
        ParticipantRecord new_rec;
        new_rec.username = username;
        new_rec.total_score = 0;
        new_rec.accepted_submissions = 0;
        new_rec.total_submissions = 0;
        new_rec.last_submission_epoch = timestamp;
        m_participants.push_back(new_rec);
        idx = (int)m_participants.size() - 1;
    }

    // Atomically increment total submissions count
    m_participants[idx].total_submissions++;
    m_participants[idx].last_submission_epoch = timestamp;

    if (is_accepted) {
        m_participants[idx].accepted_submissions++;

        // Check if problem was already solved previously
        auto& solved = m_participants[idx].solved_problem_ids;
        bool already_solved = (std::find(solved.begin(), solved.end(), problem_id) != solved.end());

        if (!already_solved) {
            solved.push_back(problem_id);
            // ATOMIC UPDATE: score += points
            m_participants[idx].total_score += points;
            first_time_solve_out = true;
        }
    }

    updated_score_out = m_participants[idx].total_score;

    // CRITICAL SECTION EXIT
    leave_critical_section();
    return true;
}

// ----------------------------------------------------------------------------
// THREAD-SAFE STATS RETRIEVAL
// ----------------------------------------------------------------------------
bool SyncLeaderboardEngine::get_participant_stats(const std::string& username, ParticipantRecord& record_out) {
    enter_critical_section();
    int idx = find_participant_index_unsafe(username);
    if (idx == -1) {
        leave_critical_section();
        return false;
    }
    record_out = m_participants[idx];
    leave_critical_section();
    return true;
}

// ----------------------------------------------------------------------------
// FORMATTED LEADERBOARD GENERATION (With Tie-Breaking)
// ----------------------------------------------------------------------------
std::string SyncLeaderboardEngine::generate_formatted_leaderboard() {
    enter_critical_section();

    // Create a local snapshot to sort without risking mutations
    std::vector<ParticipantRecord> snapshot = m_participants;

    // Sorting criteria:
    // 1. Highest total_score first (descending)
    // 2. Tie-break: Higher accepted_submissions first
    // 3. Tie-break: Earlier last_submission_epoch first
    std::sort(snapshot.begin(), snapshot.end(), [](const ParticipantRecord& a, const ParticipantRecord& b) {
        if (a.total_score != b.total_score) {
            return a.total_score > b.total_score;
        }
        if (a.accepted_submissions != b.accepted_submissions) {
            return a.accepted_submissions > b.accepted_submissions;
        }
        return a.last_submission_epoch < b.last_submission_epoch;
    });

    std::ostringstream ss;
    ss << "\n=========================================================================\n";
    ss << "                REAL-TIME CONTEST LEADERBOARD (SYNCHRONIZED)             \n";
    ss << "=========================================================================\n";
    ss << std::left 
       << std::setw(6)  << "Rank"
       << std::setw(22) << "Participant"
       << std::setw(10) << "Score"
       << std::setw(10) << "Solved"
       << std::setw(14) << "Submissions"
       << "Accuracy\n";
    ss << "-------------------------------------------------------------------------\n";

    if (snapshot.empty()) {
        ss << "                      No participants registered yet.                    \n";
    } else {
        int rank = 1;
        for (const auto& p : snapshot) {
            double accuracy = (p.total_submissions > 0) 
                ? ((double)p.accepted_submissions / (double)p.total_submissions * 100.0) 
                : 0.0;

            std::ostringstream acc_ss;
            acc_ss << std::fixed << std::setprecision(1) << accuracy << "%";

            ss << std::left
               << std::setw(6)  << rank++
               << std::setw(22) << p.username
               << std::setw(10) << p.total_score
               << std::setw(10) << (int)p.solved_problem_ids.size()
               << std::setw(14) << p.total_submissions
               << acc_ss.str() << "\n";
        }
    }
    ss << "=========================================================================\n";

    leave_critical_section();
    return ss.str();
}

// ----------------------------------------------------------------------------
// JSON LEADERBOARD GENERATION (For programmatic consumption)
// ----------------------------------------------------------------------------
std::string SyncLeaderboardEngine::generate_json_leaderboard() {
    enter_critical_section();

    std::ostringstream ss;
    ss << "{\"leaderboard\": [";
    for (size_t i = 0; i < m_participants.size(); ++i) {
        const auto& p = m_participants[i];
        ss << "{"
           << "\"user\": \"" << p.username << "\", "
           << "\"score\": " << p.total_score << ", "
           << "\"solved\": " << p.solved_problem_ids.size() << ", "
           << "\"submissions\": " << p.total_submissions
           << "}";
        if (i + 1 < m_participants.size()) ss << ", ";
    }
    ss << "]}";

    leave_critical_section();
    return ss.str();
}

// ----------------------------------------------------------------------------
// METRICS & CONTEST SUMMARY
// ----------------------------------------------------------------------------
ContestMetrics SyncLeaderboardEngine::compute_metrics() {
    enter_critical_section();

    ContestMetrics m;
    m.total_participants = (int)m_participants.size();
    m.total_submissions = 0;
    m.total_accepted = 0;
    m.average_score = 0.0;
    m.top_scorer = "N/A";
    m.highest_score = 0;

    int total_score_sum = 0;

    for (const auto& p : m_participants) {
        m.total_submissions += p.total_submissions;
        m.total_accepted += p.accepted_submissions;
        total_score_sum += p.total_score;

        if (p.total_score > m.highest_score) {
            m.highest_score = p.total_score;
            m.top_scorer = p.username;
        }
    }

    if (m.total_participants > 0) {
        m.average_score = (double)total_score_sum / (double)m.total_participants;
    }

    leave_critical_section();
    return m;
}

// ----------------------------------------------------------------------------
// DIAGNOSTICS FOR VIVA DEMONSTRATION
// ----------------------------------------------------------------------------
long long SyncLeaderboardEngine::get_lock_acquisition_count() {
    return m_lock_acquisitions;
}

long long SyncLeaderboardEngine::get_contention_events() {
    return m_contention_events;
}
