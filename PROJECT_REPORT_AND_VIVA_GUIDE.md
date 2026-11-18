# Real-Time Coding Contest Server: Project Report & OS Viva Guide

**Course**: Operating Systems Lab (`25B16CS211`) — Jaypee Institute of Information Technology (JIIT)  
**Supervisor**: Asst. Prof. Satya Prakash Patel  
**Semester**: 5th Semester (3rd Year B.Tech CSE)  

### Team Members & Contributions
1. **Akriti Singh** (`992401030302`) — Process/Thread Management & Client-Server Socket Architecture (Modules 1, 2, 3)
2. **Ayushi Tripathi** (`992401030317`) — Concurrency, Critical Section & Mutex Synchronization (Module 3)
3. **Avni Bansal** (`992401030319`) — Counting Semaphores & Submission Scheduling Queue (Module 3)
4. **Malvika Saxena** (`992401030325`) — Deadlock Avoidance (Banker's Algorithm) & Secondary Storage Logging (Modules 4, 6, 7)

---

## 1. Project Background & Motivation

In competitive programming platforms (such as Codeforces, LeetCode, or HackerRank), hundreds of participants submit code solutions simultaneously. Without proper Operating Systems mechanisms, such high-concurrency environments face severe OS failures:
- **Race Conditions**: When multiple threads update participant scores or the leaderboard concurrently, lost updates and inconsistent rankings occur.
- **Resource Exhaustion**: If the server launches an unbounded number of compilers/test runners at once, system CPU and memory are quickly overwhelmed, leading to system thrashing or crash.
- **Deadlocks**: When competing test executions request system resources (CPU cores, memory blocks, compilation slots) in conflicting order without coordination, threads can become permanently blocked in a circular wait condition.

This project implements a **Real-Time Coding Contest Server in C++** designed specifically to demonstrate and solve these core Operating System challenges using clean, textbook OS synchronization and scheduling techniques.

---

## 2. Architecture & Design

```
+-------------------------------------------------------------------------+
|                              CLIENT TIER                                |
|   Participant 1          Participant 2             Participant 3        |
|  (Akriti Singh)        (Ayushi Tripathi)          (Malvika Saxena)      |
+--------+----------------------+--------------------------+--------------+
         |                      |                          |
         +-----------------+    |    +---------------------+
                           |    |    |   (TCP Sockets on Port 8080)
                           v    v    v
+-------------------------------------------------------------------------+
|                           SERVER MASTER TIER                            |
|             Listener Socket (accept() loop)                             |
|          Spawns dedicated Worker Thread per connected client            |
+-------------------------------------------------------------------------+
                                 |
                                 v
+-------------------------------------------------------------------------+
|                       SUBMISSION SCHEDULER QUEUE                        |
|               Dispatches incoming submissions (FCFS)                    |
+-------------------------------------------------------------------------+
                                 |
                                 v
+-------------------------------------------------------------------------+
|                  COUNTING SEMAPHORE (eval_slots_sem)                    |
|             Throttles concurrent evaluations (Max Slots = 2)            |
+-------------------------------------------------------------------------+
                                 |
                                 v
+-------------------------------------------------------------------------+
|               DEADLOCK AVOIDANCE (Banker's Algorithm)                   |
|     Checks if (Available >= Requested) and preserves Safe State         |
|     Resources: CPU Cores & Memory Blocks                                |
+-------------------------------------------------------------------------+
                                 |
                                 v
+-------------------------------------------------------------------------+
|                  CRITICAL SECTION (leaderboard_mutex)                   |
|     Mutual Exclusion: Safely updates Participant Scores & Leaderboard   |
+-------------------------------------------------------------------------+
                                 |
                                 v
+-------------------------------------------------------------------------+
|                 SECONDARY STORAGE (contest_log.txt)                     |
|     Persistent Disk I/O logs every transaction with timestamp           |
+-------------------------------------------------------------------------+
```

---

## 3. Direct Mapping to Course Syllabus Modules

### Module 1: Introduction and Historical Context of OS
- **Time-Sharing & Multi-User Systems**: The server acts as a centralized time-shared system where multiple client processes share server CPU and memory resources simultaneously without interfering with each other's state.

### Module 2: Operating Structure and Architecture
- **Client-Server Architecture**: Strict separation of user-space client application (`client.cpp`) and the server subsystem (`server.cpp`).
- **Network Sockets (IPC)**: TCP/IP sockets provide standardized communication channel across processes.
- **Cross-Platform Abstraction**: Handles both Linux POSIX sockets/threads and Windows Win32 sockets/threads cleanly.

### Module 3: Process Concepts, Threads, Concurrency, Scheduling & Synchronization
- **Multithreading**:
  - The master listener thread accepts connections and invokes `os_create_thread` to spin up a dedicated worker thread (`client_worker_thread`) for each participant.
- **The Critical Section Problem**:
  - The global participant table (`g_participants`) and the scoreboard represent the critical section. If two threads modify scores simultaneously (`score += points`), a race condition occurs.
- **Mutex Locks (`os_mutex_lock` / `os_mutex_unlock`)**:
  - `leaderboard_mutex` enforces strict **Mutual Exclusion**, satisfying all 3 Critical Section requirements:
    1. *Mutual Exclusion*: Only one thread updates the scoreboard at a time.
    2. *Progress*: If no thread is in the critical section, any waiting thread can enter immediately.
    3. *Bounded Waiting*: No thread waits indefinitely.
- **Counting Semaphores (`os_sem_wait` / `os_sem_post`)**:
  - `eval_slots_sem` is initialized to `MAX_CONCURRENT_EVALUATORS = 2`.
  - Even if 10 participants submit at once, exactly 2 grading evaluations execute in parallel, and the remaining 8 wait in queue, demonstrating resource throttling.
- **Process / Job Scheduling**:
  - Submissions waiting in the queue are scheduled using **FCFS (First-Come, First-Served)** order.

### Module 4: Deadlocks
- **System Model**: Resources are categorized into distinct types: CPU Cores (`g_avail_cpu = 4`) and Memory Units (`g_avail_mem = 4`).
- **Deadlock Characterization**:
  1. *Mutual Exclusion*: Dedicated CPU/Memory tokens allocated per evaluation.
  2. *Hold and Wait*: An evaluation holds CPU tokens while requesting memory.
  3. *No Preemption*: Resources cannot be forcibly taken away during grading.
  4. *Circular Wait*: Prevented by Deadlock Avoidance.
- **Deadlock Avoidance (Banker's Algorithm)**:
  - Implemented in `bankers_request_resources()`. Before granting resources, the algorithm verifies that the system remains in a **Safe State**. If a request exceeds currently available units, the request is deferred until other evaluations release resources (`bankers_release_resources()`).

### Module 5: Memory Management
- **Buffers & Dynamic Allocation**:
  - Client request lines are dynamically parsed and allocated in memory buffers.
  - Sockets and connection state are isolated per thread to prevent cross-talk.

### Module 6: Secondary Storage Management
- **Disk File Logging (`contest_log.txt`)**:
  - Every connection, submission, scheduling decision, and score update is written to non-volatile secondary storage (`std::ofstream log_file("contest_log.txt", std::ios::app)`), protected by `console_mutex` to ensure log integrity.

### Module 7: Security & Protection
- **Authentication**:
  - Participants must authenticate (`LOGIN <username>`) before submitting solutions.
  - Commands sent by unauthenticated sockets receive `401 ERR Please LOGIN first`.

---

## 4. Comprehensive Viva Voce Questions & Answers

### Q1: What is the primary role of the Operating System demonstrated in this project?
> **Answer**: The project demonstrates how an OS coordinates multiple concurrent entities accessing shared hardware (CPU and memory) and software resources (the contest leaderboard). It implements process/thread management, inter-process communication via sockets, synchronization primitives (mutexes and semaphores), CPU scheduling, and deadlock avoidance.

### Q2: Why did you use both a Mutex AND a Semaphore? What is the difference?
> **Answer**:
> - **Mutex (`leaderboard_mutex`)**: A binary mutual exclusion locking mechanism used to protect the **Critical Section** (the participant score table). Only one thread can lock it at any given moment. The thread that locks it must unlock it.
> - **Counting Semaphore (`eval_slots_sem`)**: A signaling mechanism initialized with an integer value (`MAX_CONCURRENT_EVALUATORS = 2`). It manages a pool of identical resources (the grading engines). It allows up to 2 threads to enter the evaluation phase simultaneously. Any thread can signal (`sem_post`) to wake up a waiting thread.

### Q3: What is a Race Condition, and where would it occur in your code without a Mutex?
> **Answer**: A race condition occurs when two or more threads attempt to read and write to shared data concurrently, and the final outcome depends on the timing/order of thread execution.  
> In our server, if participant A and participant B submit solutions at the exact same millisecond:
> ```cpp
> p.score += prob->points; // Non-atomic read-modify-write!
> ```
> At the assembly level, this involves:
> 1. `MOV EAX, [score]`
> 2. `ADD EAX, 20`
> 3. `MOV [score], EAX`  
> If an interrupt or context switch occurs between step 1 and 3, one participant's points would be overwritten and lost. Enclosing this in `os_mutex_lock(&leaderboard_mutex)` ensures atomicity.

### Q4: How does your project avoid Deadlocks? Which algorithm did you use?
> **Answer**: We implemented **Deadlock Avoidance** using **Banker's Algorithm** principles (Module 4). The system models evaluation resources as CPU units and Memory units. When an evaluation job requests resources, `bankers_request_resources()` checks if `available >= requested`. If allocating the resources keeps the system in a **Safe State**, the request is granted. If not, the request is safely deferred, preventing the 4th Coffman condition (**Circular Wait**).

### Q5: How are client requests scheduled?
> **Answer**: Submissions are queued with an arrival timestamp and priority number. The server scheduler dispatches them using **First-Come, First-Served (FCFS)** policy.

### Q6: How does the server handle multiple clients at the same time?
> **Answer**: The server creates a master listening TCP socket on port 8080. When `accept()` receives an incoming client connection, it spawns a dedicated worker thread (`client_worker_thread`) using `os_create_thread`. Each client runs independently in its own thread of execution without blocking other participants.

### Q7: How does your code run on both Windows and Linux?
> **Answer**: We implemented a lightweight preprocessor abstraction layer (`#ifdef _WIN32 ... #else ... #endif`). On Linux, it directly maps to POSIX Threads (`pthread_create`, `pthread_mutex_t`, `sem_t`) and BSD Sockets. On Windows, it maps to Win32 threads (`CreateThread`), `CRITICAL_SECTION`, and Winsock2. This ensures full standards compliance with zero external dependencies.

---

## 5. Team Member Presentation Guide (Who Speaks What)

When presenting to Asst. Prof. Satya Prakash Patel, divide the viva presentation cleanly:

- **Akriti Singh**:
  - Presents: Sockets, Client-Server connection architecture, and the Master listener thread.
  - Key terms: `socket()`, `bind()`, `listen()`, `accept()`, thread per client model.
- **Ayushi Tripathi**:
  - Presents: Concurrency, Critical Section problem, and Mutex locks.
  - Key terms: Race condition prevention, `leaderboard_mutex`, atomicity of score updates.
- **Avni Bansal**:
  - Presents: Counting Semaphores and Submission Queue Scheduling.
  - Key terms: `eval_slots_sem`, `os_sem_wait`, `os_sem_post`, throttling to 2 evaluation slots, FCFS scheduling.
- **Malvika Saxena**:
  - Presents: Deadlock Avoidance (Banker's Algorithm), Secondary Storage logging, and Access Control.
  - Key terms: Safe state check, `g_avail_cpu`, `g_avail_mem`, `contest_log.txt`, authentication.

---

## 6. Verification and Test Results

The concurrency and synchronization of the system was verified using `demo.py`, which simulated all 4 participants submitting concurrently:
- **Concurrent Connections**: 4 clients connected simultaneously.
- **Semaphore Limiting**: Exactly 2 clients (`Malvika_Saxena` and `Ayushi_Tripathi`) were granted evaluation slots; `Akriti_Singh` and `Avni_Bansal` waited until a slot was released.
- **Banker's Safety Check**: Resources were allocated safely and verified in real time (`State: SAFE`).
- **Critical Section Integrity**: Final leaderboard correctly tallied scores with zero race conditions or lost updates.
- **Persistence**: All operations successfully recorded to `contest_log.txt`.
