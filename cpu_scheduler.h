#ifndef CPU_SCHEDULER_H
#define CPU_SCHEDULER_H

/**
 * ============================================================================
 * MODULE 3: TASK SCHEDULING & CONCURRENCY THROTTLING ENGINE
 * Author: Avni Bansal (Enrollment ID: 992401030319)
 * Course: Operating Systems Lab (25B16CS211) - JIIT
 * Supervisor: Asst. Prof. Satya Prakash Patel
 *
 * Covers OS Syllabus:
 *  - Module 3: Scheduling Criteria, Scheduling Algorithms (FCFS, Priority),
 *              Counting Semaphores (Concurrency Throttling), Burst Time,
 *              Waiting Time, and Turnaround Time Metrics.
 * ============================================================================
 */

#include <string>
#include <vector>
#include <queue>

#ifdef _WIN32
    #include <windows.h>
    typedef HANDLE os_semaphore_t;
    typedef CRITICAL_SECTION sched_mutex_t;
#else
    #include <semaphore.h>
    #include <pthread.h>
    typedef sem_t os_semaphore_t;
    typedef pthread_mutex_t sched_mutex_t;
#endif

enum SchedulingPolicy {
    POLICY_FCFS = 1,
    POLICY_PRIORITY = 2,
    POLICY_ROUND_ROBIN = 3
};

struct SubmissionTask {
    int task_id;
    std::string student_name;
    int problem_id;
    std::string code_snippet;
    int priority;           // Higher integer = higher priority
    int estimated_burst_ms; // Simulated execution time
    long long arrival_time_ms;
    long long start_time_ms;
    long long completion_time_ms;
};

struct SchedulerStats {
    int total_tasks_scheduled;
    int completed_tasks;
    double average_waiting_time_ms;
    double average_turnaround_time_ms;
};

class CPUSchedulerEngine {
public:
    CPUSchedulerEngine(int max_evaluator_slots, SchedulingPolicy policy);
    ~CPUSchedulerEngine();

    // Submission Enqueueing & Dispatching
    int enqueue_task(
        const std::string& student, 
        int problem_id, 
        const std::string& code, 
        int priority, 
        int burst_ms
    );
    bool dispatch_next_task(SubmissionTask& task_out);

    // Semaphore-based Evaluation Slot Throttling
    void acquire_evaluator_slot();
    void release_evaluator_slot();

    // Policy Configuration
    void set_scheduling_policy(SchedulingPolicy policy);
    SchedulingPolicy get_scheduling_policy() const;
    std::string get_policy_name() const;

    // Execution Lifecycle & Performance Metrics
    void mark_task_started(int task_id, long long start_ms);
    void mark_task_completed(int task_id, long long completion_ms);
    SchedulerStats get_scheduler_stats();

    // Queue Inspection
    int get_queue_depth();
    std::string get_queue_status_report();

private:
    int m_max_slots;
    SchedulingPolicy m_policy;
    int m_task_id_generator;

    // Queue storage
    std::vector<SubmissionTask> m_pending_tasks;
    std::vector<SubmissionTask> m_history_tasks;

    // Synchronization primitives
    os_semaphore_t m_eval_semaphore;
    sched_mutex_t m_queue_mutex;

    void init_synchronization();
    void destroy_synchronization();
    void lock_queue();
    void unlock_queue();

    void sort_queue_by_policy();
};

#endif // CPU_SCHEDULER_H
