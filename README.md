# Real-Time Coding Contest Server (OS Lab Project)

**Course**: Operating Systems Lab (`25B16CS211`) — Jaypee Institute of Information Technology (JIIT)  
**Supervisor**: Asst. Prof. Satya Prakash Patel  
**Team Members**:
- Akriti Singh (`992401030302`)
- Ayushi Tripathi (`992401030317`)
- Avni Bansal (`992401030319`)
- Malvika Saxena (`992401030325`)

---

## 📌 Project Overview

This project implements a lightweight, high-performance **Real-Time Coding Contest Server** in C++. It directly demonstrates core Operating System principles covered in your course syllabus:

1. **Multi-Threading & Sockets** (Module 2 & 3): Master listener thread accepts TCP connections; dedicated worker threads handle each participant concurrently.
2. **Critical Section & Mutex Locks** (Module 3): Shared contest state and leaderboard are protected with mutual exclusion (`os_mutex_lock`), preventing race conditions when multiple participants submit at the same millisecond.
3. **Counting Semaphores** (Module 3): `os_sem_wait` and `os_sem_post` throttle code evaluation engines to a fixed maximum concurrent capacity (`MAX_CONCURRENT_EVALUATORS = 2`), queuing extra submissions.
4. **CPU Scheduling Algorithms** (Module 3): First-Come First-Served (FCFS) and Priority submission queue management.
5. **Deadlock Avoidance** (Module 4): Implements **Banker's Algorithm** safe-state checks before allocating grading resources (CPU cores and Memory tokens) to prevent deadlock.
6. **Secondary Storage & Persistence** (Module 6): Disk logging to `contest_log.txt` records all events chronologically.
7. **Security & Authentication** (Module 7): Client authentication before granting contest access.

---

## 📁 Clean & Simple File Structure

The project is intentionally kept simple with **only 3 code files** so it is easy to navigate, understand, and explain during evaluations:

```text
Os_Simple_project/
├── server.cpp                      # Central Contest Server (Sockets, Mutex, Semaphore, Banker's Algo)
├── client.cpp                      # Interactive Participant Client (Terminal Menu & Auto-mode)
├── demo.py                         # 1-Click Concurrency Simulation (Tests all 4 students simultaneously)
├── contest_log.txt                 # Persistent disk log generated during execution
├── PROJECT_REPORT_AND_VIVA_GUIDE.md # Complete viva preparation & syllabus mapping guide
└── README.md                       # This quick-start guide
```

---

## 🚀 How to Run

### Method 1: Instant 1-Click Simulation (Recommended for Viva Demo)
Demonstrates the server running and all 4 team members (`Akriti`, `Ayushi`, `Avni`, `Malvika`) submitting solutions simultaneously:

```bash
# Compile server & client first
g++ server.cpp -lws2_32 -o server.exe
g++ client.cpp -lws2_32 -o client.exe

# Run the automated multi-client concurrency demo
python demo.py
```
*(On Linux: replace `-lws2_32` with `-pthread`)*

---

### Method 2: Manual Interactive Mode

#### Terminal 1 — Start the Server:
```bash
# Windows:
g++ server.cpp -lws2_32 -o server.exe
.\server.exe

# Linux:
g++ -pthread server.cpp -o server
./server
```

#### Terminal 2 — Start Participant Client:
```bash
# Windows:
g++ client.cpp -lws2_32 -o client.exe
.\client.exe

# Linux:
g++ client.cpp -o client
./client
```

Inside the client, you will see an interactive menu:
```text
=======================================================
        REAL-TIME CODING CONTEST PARTICIPANT           
        Logged in as: [Malvika_Saxena]
=======================================================
  [1] View Contest Problems
  [2] Submit Solution to a Problem
  [3] View Live Synchronized Leaderboard
  [4] View My Current Score & Solved Count
  [5] Exit / Logout
-------------------------------------------------------
Select an option (1-5):
```

---

## 💡 Course Syllabus Coverage Summary

| Course Module | Concept in Syllabus | Implementation in Project |
|---|---|---|
| **Module 1 & 2** | Time-Sharing & Client-Server Architecture | Multi-user TCP socket server handling multiple simultaneous participants |
| **Module 3** | Threads & Concurrency | Master listener thread + dedicated worker thread per connected client |
| **Module 3** | Critical Section & Mutex | `os_mutex_lock(&leaderboard_mutex)` protects participant scores from race conditions |
| **Module 3** | Semaphores & Resource Limiting | `os_sem_wait(&eval_slots_sem)` limits concurrent grading engines to 2 |
| **Module 3** | CPU Scheduling | Submissions queued and dispatched using FCFS & Priority scheduling |
| **Module 4** | Deadlock Avoidance | `bankers_request_resources()` checks system safe state before allocating execution tokens |
| **Module 6** | Secondary Storage & File Systems | `contest_log.txt` records all transactions persistently to disk |
| **Module 7** | Protection & Access Control | Mandatory `LOGIN` verification before allowing submissions |

---

For in-depth explanations and viva question answers, see [PROJECT_REPORT_AND_VIVA_GUIDE.md](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/Os_Simple_project/PROJECT_REPORT_AND_VIVA_GUIDE.md).
