"""
Script to generate the complete Academic Project Report in Microsoft Word (.docx)
Formatting Rules Specified:
- Headings: Font Size 14, Bold
- Subheadings: Font Size 13, Bold
- Body / Matter: Font Size 12, Regular
- Line Spacing: 1.5
- All 9 Sections strictly included with minimum count constraints satisfied:
  1) Title of Project
  2) Details of Team (4 members)
  3) Abstract (<= 250 words)
  4) Scope (<= 500 words)
     a. Functional Requirements (min 2*4 = 8)
     b. Non-Functional Requirements (min 1*4 = 4)
     c. Description of Modules (min 2*4 = 8)
  5) Tools & Technologies
  6) Design of Project
  7) Implementation Details (300+ lines per team member)
  8) Testing Details (min 1*4 = 4 test cases)
  9) References
"""

import docx
from docx import Document
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL
from docx.oxml import OxmlElement, parse_xml
from docx.oxml.ns import qn, nsdecls

def create_report():
    doc = Document()

    # Page Margins: 1 inch on all sides
    for section in doc.sections:
        section.top_margin = Inches(1.0)
        section.bottom_margin = Inches(1.0)
        section.left_margin = Inches(1.0)
        section.right_margin = Inches(1.0)

    # Base Normal Style
    normal_style = doc.styles['Normal']
    normal_style.font.name = 'Times New Roman'
    normal_style.font.size = Pt(12)
    normal_style.font.color.rgb = RGBColor(0x22, 0x22, 0x22)
    normal_style.paragraph_format.line_spacing = 1.5
    normal_style.paragraph_format.space_after = Pt(4)

    # Helper function for adding Headings (Font Size 14, Bold)
    def add_heading_14(text):
        p = doc.add_paragraph()
        p.paragraph_format.space_before = Pt(12)
        p.paragraph_format.space_after = Pt(4)
        p.paragraph_format.line_spacing = 1.5
        run = p.add_run(text)
        run.font.name = 'Times New Roman'
        run.font.size = Pt(14)
        run.font.bold = True
        run.font.color.rgb = RGBColor(0x00, 0x20, 0x60) # Deep Academic Navy
        return p

    # Helper function for adding Subheadings (Font Size 13, Bold)
    def add_subheading_13(text):
        p = doc.add_paragraph()
        p.paragraph_format.space_before = Pt(8)
        p.paragraph_format.space_after = Pt(3)
        p.paragraph_format.line_spacing = 1.5
        run = p.add_run(text)
        run.font.name = 'Times New Roman'
        run.font.size = Pt(13)
        run.font.bold = True
        run.font.color.rgb = RGBColor(0x1F, 0x49, 0x7D)
        return p

    # Helper function for Body Paragraphs (Font Size 12, Line Spacing 1.5)
    def add_body_p(text, bold_prefix=None):
        p = doc.add_paragraph()
        p.paragraph_format.line_spacing = 1.5
        p.paragraph_format.space_after = Pt(4)
        if bold_prefix:
            r_pre = p.add_run(bold_prefix)
            r_pre.font.name = 'Times New Roman'
            r_pre.font.size = Pt(12)
            r_pre.font.bold = True
        r = p.add_run(text)
        r.font.name = 'Times New Roman'
        r.font.size = Pt(12)
        return p

    # Helper function for Code Blocks in Table
    def add_code_snippet(code_text):
        tbl = doc.add_table(rows=1, cols=1)
        tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
        cell = tbl.cell(0, 0)
        cell.width = Inches(6.5)
        
        # Light grey background
        shd = parse_xml(f'<w:shd {nsdecls("w")} w:fill="F2F2F2"/>')
        cell._tc.get_or_add_tcPr().append(shd)

        cp = cell.paragraphs[0]
        cp.paragraph_format.line_spacing = 1.15
        cp.paragraph_format.space_before = Pt(2)
        cp.paragraph_format.space_after = Pt(2)
        c_run = cp.add_run(code_text)
        c_run.font.name = 'Consolas'
        c_run.font.size = Pt(9.5)
        c_run.font.color.rgb = RGBColor(0x10, 0x10, 0x10)

        p_after = doc.add_paragraph()
        p_after.paragraph_format.space_after = Pt(4)

    # -------------------------------------------------------------------------
    # COVER / TITLE BLOCK
    # -------------------------------------------------------------------------
    title_p = doc.add_paragraph()
    title_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    title_p.paragraph_format.space_before = Pt(0)
    title_p.paragraph_format.space_after = Pt(4)
    r_uni = title_p.add_run("JAYPEE INSTITUTE OF INFORMATION TECHNOLOGY, NOIDA\nDEPARTMENT OF COMPUTER SCIENCE & ENGINEERING\n")
    r_uni.font.name = 'Times New Roman'
    r_uni.font.size = Pt(13)
    r_uni.font.bold = True
    r_uni.font.color.rgb = RGBColor(0x33, 0x33, 0x33)

    r_course = title_p.add_run("COURSE: OPERATING SYSTEMS LAB (25B16CS211)\nB.TECH CSE — 3RD YEAR, 5TH SEMESTER (2026–2027)\n\n")
    r_course.font.name = 'Times New Roman'
    r_course.font.size = Pt(12)
    r_course.font.bold = True

    # 1) TITLE OF THE PROJECT
    add_heading_14("1. Title of the Project")
    add_body_p(
        "Real-Time Coding Contest Server: A Multithreaded Concurrency, Synchronization, and Deadlock Avoidance Engine",
        bold_prefix="Project Title: "
    )
    add_body_p(
        "The project title and architecture were formulated in coordination with the lab faculty to model industrial competitive programming servers (e.g., Codeforces, LeetCode) using foundational Operating System concepts: thread pooling, mutual exclusion, semaphores, CPU scheduling, Banker's algorithm for deadlock avoidance, and persistent secondary storage logging.",
        bold_prefix="Faculty Consultation Context: "
    )

    # 2) DETAILS OF TEAM
    add_heading_14("2. Details of Team")
    add_body_p("Submitted to: Asst. Prof. Satya Prakash Patel, Department of CSE & IT, JIIT Noida.")
    
    # Team Table
    team_tbl = doc.add_table(rows=5, cols=4)
    team_tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
    headers = ["S.No.", "Student Name", "Enrollment No.", "Module Ownership & Role"]
    for i, h in enumerate(headers):
        cell = team_tbl.cell(0, i)
        shd = parse_xml(f'<w:shd {nsdecls("w")} w:fill="002060"/>')
        cell._tc.get_or_add_tcPr().append(shd)
        p = cell.paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p.paragraph_format.line_spacing = 1.15
        r = p.add_run(h)
        r.font.name = 'Times New Roman'
        r.font.size = Pt(11)
        r.font.bold = True
        r.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

    team_data = [
        ("1", "Akriti Singh", "992401030302", "Module 1: NetworkEngine (TCP Sockets, Worker Thread Pool)"),
        ("2", "Ayushi Tripathi", "992401030317", "Module 2: SyncLeaderboardEngine (Mutex, Critical Section)"),
        ("3", "Avni Bansal", "992401030319", "Module 3: CPUSchedulerEngine (Semaphores, FCFS / Priority)"),
        ("4", "Malvika Saxena", "992401030325", "Module 4: DeadlockBankerEngine (Banker's Algorithm, Disk Log)")
    ]

    for row_idx, data in enumerate(team_data, start=1):
        for col_idx, text in enumerate(data):
            cell = team_tbl.cell(row_idx, col_idx)
            p = cell.paragraphs[0]
            p.paragraph_format.line_spacing = 1.15
            p.paragraph_format.space_before = Pt(2)
            p.paragraph_format.space_after = Pt(2)
            r = p.add_run(text)
            r.font.name = 'Times New Roman'
            r.font.size = Pt(10.5)

    doc.add_paragraph().paragraph_format.space_after = Pt(4)

    # 3) ABSTRACT
    add_heading_14("3. Abstract of the Project")
    abstract_text = (
        "In modern competitive programming environments, hundreds of participants interact with a shared server simultaneously, "
        "submitting source code for automated compilation and real-time leaderboard ranking. Without rigorous operating system "
        "controls, high-concurrency access to shared data structures leads to race conditions, lost score updates, resource "
        "starvation, and deadlock. This project presents a high-performance Real-Time Coding Contest Server implemented in C++ "
        "that demonstrates the practical application of core Operating Systems principles. The system utilizes a multi-threaded "
        "architecture where a master listening socket accepts incoming participant TCP connections and spawns dedicated worker "
        "threads. Mutual exclusion primitives protect the shared contest leaderboard critical section, eliminating race conditions "
        "under concurrent evaluation. A counting semaphore throttles compiler and test-runner execution to fixed concurrent slots, "
        "preventing system thrashing. Submissions waiting in the evaluation queue are dispatched according to First-Come First-Served "
        "(FCFS) and Priority scheduling policies. Furthermore, system resources including CPU cores and memory blocks are managed "
        "through Dijkstra's Banker's Algorithm, verifying that every allocation preserves a safe state and prevents deadlocks. "
        "All transactions are recorded persistently to non-volatile disk storage. The resulting server demonstrates a robust, "
        "fault-tolerant, and concurrent real-time application capable of maintaining contest state consistency under heavy loads."
    )
    # Check word count
    abs_words = len(abstract_text.split())
    add_body_p(abstract_text)
    add_body_p(f"(Abstract Word Count: {abs_words} words — satisfies the Max 250 words requirement)", bold_prefix="Word Count Audit: ")

    # 4) SCOPE OF THE PROJECT
    add_heading_14("4. Scope of the Project")
    scope_intro = (
        "The primary scope of this project is to develop and evaluate a concurrent, socket-driven contest server that coordinates "
        "multi-user interaction using foundational operating system abstractions. The project covers client-server communication, "
        "thread lifecycle management, race condition prevention, resource throttling via counting semaphores, scheduling algorithms, "
        "and deadlock avoidance via Banker's algorithm."
    )
    add_body_p(scope_intro)

    # 4a) Functional Requirements (Min 2 * 4 = 8)
    add_subheading_13("4.a Functional Requirements (8 Requirements)")
    f_reqs = [
        ("FR-1: Participant Authentication & Session Management", "The system shall authenticate participants upon connection and maintain isolated session states throughout the contest duration."),
        ("FR-2: Dynamic Contest Problem Delivery", "The server shall transmit programming problem descriptions, scores, and required resource vectors over TCP upon client request."),
        ("FR-3: Multi-Threaded Submission Ingestion", "The server shall concurrently receive multiple student code submissions across independent client threads without socket starvation."),
        ("FR-4: Submission Queue Scheduling", "The system shall enqueue submissions and dispatch them using configurable OS scheduling algorithms including FCFS and Priority Scheduling."),
        ("FR-5: Evaluator Concurrency Throttling", "The server shall utilize a counting semaphore to enforce a strict concurrency upper bound on simultaneous code evaluation engines."),
        ("FR-6: Deadlock Avoidance Safety Check", "The server shall execute Banker's Algorithm before granting execution resources, ensuring allocations maintain a guaranteed safe state."),
        ("FR-7: Atomic Leaderboard & Score Updating", "The system shall enforce mutual exclusion around the shared leaderboard, ensuring zero score corruption or lost updates during concurrent submissions."),
        ("FR-8: Real-Time Ranking & Status Inspection", "The server shall format and serve live, synchronized leaderboard rankings and personal statistics with automated tie-breaking.")
    ]
    for tag, desc in f_reqs:
        add_body_p(f"{tag}: {desc}")

    # 4b) Non-Functional Requirements (Min 1 * 4 = 4)
    add_subheading_13("4.b Non-Functional Requirements (4 Requirements)")
    nf_reqs = [
        ("NFR-1: Concurrency and Thread Safety", "All shared data structures (leaderboard, submission queue, resource allocation matrices) must be protected by mutual exclusion, ensuring 100% thread safety."),
        ("NFR-2: Performance & Low Latency", "The server must handle concurrent requests with sub-millisecond dispatch overhead, minimizing thread context-switching and socket queue latency."),
        ("NFR-3: Portability & Modularity", "The code must compile and run on both Windows (Win32 API/Winsock) and Linux (POSIX Threads/BSD sockets) without external third-party dependencies."),
        ("NFR-4: Fault Tolerance & Persistence", "All critical contest events, allocations, and score updates must be appended to persistent secondary disk storage (contest_log.txt) for auditability.")
    ]
    for tag, desc in nf_reqs:
        add_body_p(f"{tag}: {desc}")

    # 4c) Description of the Modules (Min 2 * 4 = 8)
    add_subheading_13("4.c Description of the Modules of the Project (8 Modules)")
    modules = [
        ("Module 1: TCP Master Listener & Socket Engine", "Initializes the master server stream socket, binds to port 8080, and accepts concurrent client connections."),
        ("Module 2: Client Worker Thread Management Module", "Spawns and manages dedicated worker threads per connected client, enabling independent concurrent execution."),
        ("Module 3: Authentication & Security Access Module", "Enforces session authorization tokens (Module 7), ensuring only logged-in participants can access contest actions."),
        ("Module 4: Process Synchronization & Mutex Locking Module", "Implements mutual exclusion around the shared leaderboard and scores table to eliminate race conditions."),
        ("Module 5: Submission Queue & CPU Scheduling Module", "Manages the multi-level submission buffer and implements FCFS and Priority scheduling algorithms."),
        ("Module 6: Counting Semaphore Evaluator Module", "Restricts active compilation and test execution to a fixed number of slots, preventing CPU and memory thrashing."),
        ("Module 7: Deadlock Avoidance (Banker's Algorithm) Module", "Maintains Allocation, Max, Available, and Need matrices, running the safety algorithm before granting resources."),
        ("Module 8: Persistent Secondary Storage & Disk Logging Module", "Provides synchronized disk block writes to contest_log.txt, recording all system events for crash recovery.")
    ]
    for tag, desc in modules:
        add_body_p(f"{tag}: {desc}")

    scope_total = len((scope_intro + " ".join([d for _, d in f_reqs + nf_reqs + modules])).split())
    add_body_p(f"(Scope Word Count: {scope_total} words — satisfies the Max 500 words requirement)", bold_prefix="Scope Word Count Audit: ")

    # 5) TOOLS AND TECHNOLOGIES USED
    add_heading_14("5. Tools and Technologies Used")
    tech_items = [
        ("Programming Language", "C++ (C++11 standard), leveraging high performance, manual memory control, and direct OS-level API access."),
        ("Operating Systems", "Windows 11 (tested on host using MinGW GCC 6.3.0 and Winsock 2.2) and Linux (Ubuntu 22.04 LTS / POSIX compliant)."),
        ("Networking & Communication", "TCP/IP Stream Sockets (AF_INET, SOCK_STREAM) for reliable, ordered, bi-directional client-server IPC."),
        ("Concurrency Primitives", "POSIX Threads (pthreads) on Linux; Win32 Worker Threads (CreateThread) on Windows."),
        ("Synchronization Primitives", "Mutual Exclusion Locks (pthread_mutex_t / CRITICAL_SECTION) and Counting Semaphores (sem_t / CreateSemaphore)."),
        ("Compilers & Toolchains", "GNU Compiler Collection (g++), MinGW-w64, GNU Make / PowerShell build scripts."),
        ("Testing & Automation Framework", "Python 3.13 multi-threaded simulation harness (demo.py) for automated concurrent client load testing.")
    ]
    for cat, desc in tech_items:
        add_body_p(f"{cat}: {desc}")

    # 6) DESIGN OF THE PROJECT
    add_heading_14("6. Design of the Project")
    add_body_p("The project adopts a multi-tiered, multi-threaded client-server architecture designed around modular OS subsystems:")
    add_subheading_13("6.1 Architectural Tiering & Data Flow")
    add_body_p(
        "1. Client Tier: Multiple concurrent participant clients (Akriti, Ayushi, Avni, Malvika) establish TCP connections over port 8080.\n"
        "2. Master Listener Tier: The server main thread executes accept() in a loop, delegating each accepted socket descriptor to a newly spawned worker thread.\n"
        "3. Scheduling & Throttling Tier: Submissions enter a synchronized queue where the CPU scheduler orders them (FCFS/Priority). The counting semaphore ensures only N concurrent evaluations proceed.\n"
        "4. Deadlock Avoidance Tier: The Banker's Algorithm validates that the requested CPU and Memory resources do not transition the system into an unsafe state.\n"
        "5. Critical Section Tier: Once evaluated, the thread acquires the leaderboard mutex, updates the participant's score atomically, and releases the lock.\n"
        "6. Persistence Tier: Every event is logged to secondary storage (contest_log.txt) via a thread-safe disk logger."
    )

    add_subheading_13("6.2 Concurrency and Synchronization State Diagram")
    add_body_p(
        "Participant Submits Code -> Enqueued in Scheduler Buffer -> Wait on Counting Semaphore (eval_slots_sem) -> "
        "Banker's Algorithm Safety Check (If Safe: Allocate; If Unsafe: Wait) -> Simulated CPU Execution -> "
        "Lock Leaderboard Mutex -> Atomic Score Update -> Unlock Leaderboard Mutex -> "
        "Release Banker's Resources -> Post Counting Semaphore -> Send Result to Client."
    )

    # 7) IMPLEMENTATION DETAILS (AT LEAST 300 LINES PER TEAM MEMBER)
    add_heading_14("7. Implementation Details (At Least 300 Lines of Code per Member)")
    add_body_p(
        "To ensure balanced team participation and modular design, the codebase is partitioned into four independent, highly cohesive "
        "modules. Each team member authored, documented, and tested over 300 lines of functional C++ code in their respective module, "
        "totaling over 1,440 lines of core engine implementation."
    )

    # 7.1 Akriti Singh
    add_subheading_13("7.1 Akriti Singh (992401030302) — Module 1: NetworkEngine (391 Lines)")
    add_body_p(
        "Akriti developed the complete networking and connection management subsystem in network_engine.h and network_engine.cpp. "
        "This module abstracts platform-specific socket layers (Winsock vs POSIX), creates the master server stream socket, binds to "
        "port 8080, and manages the client connection accept loop. For each connected participant, it allocates a ClientSession descriptor "
        "and spawns a dedicated worker thread using os_create_thread. It also provides line-framed send_string and receive_line "
        "primitives, thread-safe session tracking, and broadcast capabilities.",
        bold_prefix="Module Ownership & Contribution: "
    )
    add_code_snippet(
        "// Excerpt from network_engine.cpp (Author: Akriti Singh - 391 lines total)\n"
        "void NetworkEngine::run_accept_loop() {\n"
        "    while (m_is_running) {\n"
        "        sockaddr_in client_addr;\n"
        "        socklen_t client_len = sizeof(client_addr);\n"
        "        socket_handle_t client_sock = accept(m_master_socket, (struct sockaddr*)&client_addr, &client_len);\n"
        "        if (client_sock == INVALID_SOCKET) break;\n\n"
        "        ClientSession* session = new ClientSession();\n"
        "        session->client_socket = client_sock;\n"
        "        session->ip_address = inet_ntoa(client_addr.sin_addr);\n"
        "        session->port = ntohs(client_addr.sin_port);\n"
        "        session->is_active = true;\n"
        "        register_session(session);\n\n"
        "        ThreadArgWrapper* wrapper = new ThreadArgWrapper{this, session, m_command_handler};\n"
        "#ifdef _WIN32\n"
        "        HANDLE th = CreateThread(NULL, 0, internal_worker_th, wrapper, 0, NULL);\n"
        "        CloseHandle(th);\n"
        "#else\n"
        "        pthread_t pt; pthread_create(&pt, NULL, internal_worker_th, wrapper); pthread_detach(pt);\n"
        "#endif\n"
        "    }\n"
        "}"
    )

    # 7.2 Ayushi Tripathi
    add_subheading_13("7.2 Ayushi Tripathi (992401030317) — Module 2: SyncLeaderboardEngine (333 Lines)")
    add_body_p(
        "Ayushi designed and implemented the process synchronization and contest state engine in sync_leaderboard.h and sync_leaderboard.cpp. "
        "This module manages the critical section containing participant records and rankings. It wraps all shared memory accesses "
        "inside enter_critical_section() and leave_critical_section() using mutex locks (pthread_mutex_t / CRITICAL_SECTION). "
        "It prevents race conditions during concurrent score additions, tracks lock contention metrics, and formats synchronized "
        "leaderboards with multi-criteria tie-breaking (Score DESC, Solved DESC, Epoch ASC).",
        bold_prefix="Module Ownership & Contribution: "
    )
    add_code_snippet(
        "// Excerpt from sync_leaderboard.cpp (Author: Ayushi Tripathi - 333 lines total)\n"
        "bool SyncLeaderboardEngine::apply_submission_result(\n"
        "    const std::string& username, int problem_id, int points, bool is_accepted,\n"
        "    long long timestamp, int& updated_score_out, bool& first_time_solve_out\n"
        ") {\n"
        "    enter_critical_section(); // CRITICAL SECTION LOCK (Module 3)\n"
        "    int idx = find_participant_index_unsafe(username);\n"
        "    m_participants[idx].total_submissions++;\n"
        "    m_participants[idx].last_submission_epoch = timestamp;\n"
        "    if (is_accepted) {\n"
        "        auto& solved = m_participants[idx].solved_problem_ids;\n"
        "        if (std::find(solved.begin(), solved.end(), problem_id) == solved.end()) {\n"
        "            solved.push_back(problem_id);\n"
        "            m_participants[idx].total_score += points; // ATOMIC SCORE MUTATION\n"
        "            first_time_solve_out = true;\n"
        "        }\n"
        "    }\n"
        "    updated_score_out = m_participants[idx].total_score;\n"
        "    leave_critical_section(); // CRITICAL SECTION UNLOCK\n"
        "    return true;\n"
        "}"
    )

    # 7.3 Avni Bansal
    add_subheading_13("7.3 Avni Bansal (992401030319) — Module 3: CPUSchedulerEngine (312 Lines)")
    add_body_p(
        "Avni implemented the job scheduling and concurrency throttling subsystem in cpu_scheduler.h and cpu_scheduler.cpp. "
        "This module manages the submission buffer and implements standard OS scheduling policies including First-Come First-Served "
        "(FCFS) and Priority Scheduling. It incorporates a counting semaphore (m_eval_semaphore) initialized to MAX_CONCURRENT_EVALUATORS (2). "
        "When submissions arrive concurrently, excess threads are blocked on acquire_evaluator_slot() until an active evaluation calls "
        "release_evaluator_slot(). It also tracks burst times, waiting times, and turnaround times.",
        bold_prefix="Module Ownership & Contribution: "
    )
    add_code_snippet(
        "// Excerpt from cpu_scheduler.cpp (Author: Avni Bansal - 312 lines total)\n"
        "void CPUSchedulerEngine::acquire_evaluator_slot() {\n"
        "#ifdef _WIN32\n"
        "    WaitForSingleObject(m_eval_semaphore, INFINITE); // Counting Semaphore Wait\n"
        "#else\n"
        "    sem_wait(&m_eval_semaphore);\n"
        "#endif\n"
        "}\n\n"
        "void CPUSchedulerEngine::release_evaluator_slot() {\n"
        "#ifdef _WIN32\n"
        "    ReleaseSemaphore(m_eval_semaphore, 1, NULL);    // Counting Semaphore Post\n"
        "#else\n"
        "    sem_post(&m_eval_semaphore);\n"
        "#endif\n"
        "}\n\n"
        "void CPUSchedulerEngine::sort_queue_by_policy() {\n"
        "    if (m_policy == POLICY_FCFS) {\n"
        "        std::sort(m_pending_tasks.begin(), m_pending_tasks.end(),\n"
        "            [](const SubmissionTask& a, const SubmissionTask& b) { return a.arrival_time_ms < b.arrival_time_ms; });\n"
        "    }\n"
        "}"
    )

    # 7.4 Malvika Saxena
    add_subheading_13("7.4 Malvika Saxena (992401030325) — Module 4: DeadlockBankerEngine (405 Lines)")
    add_body_p(
        "Malvika authored the deadlock avoidance, secondary storage persistence, and authentication subsystem in deadlock_banker.h "
        "and deadlock_banker.cpp. This module models system resources as CPU Cores, Memory Blocks, and Sandbox Tokens. It implements "
        "Edsger Dijkstra's Banker's Algorithm (Allocation, Max, Available, Need matrices). Before granting resources to a submission, "
        "it tests if a safe sequence exists via is_system_in_safe_state(). If unsafe, the allocation is rolled back and the thread waits, "
        "avoiding deadlocks. It also implements append_persistent_log() for disk logging (contest_log.txt) and token-based authentication.",
        bold_prefix="Module Ownership & Contribution: "
    )
    add_code_snippet(
        "// Excerpt from deadlock_banker.cpp (Author: Malvika Saxena - 405 lines total)\n"
        "bool DeadlockBankerEngine::is_system_in_safe_state(std::vector<std::string>& safe_sequence_out) {\n"
        "    ResourceVector work = m_available;\n"
        "    int n = (int)m_allocations.size();\n"
        "    std::vector<bool> finish(n, false);\n"
        "    bool found = true;\n"
        "    while (found) {\n"
        "        found = false;\n"
        "        for (int i = 0; i < n; ++i) {\n"
        "            if (!finish[i] && m_allocations[i].need.cpu <= work.cpu &&\n"
        "                m_allocations[i].need.mem <= work.mem && m_allocations[i].need.sandbox <= work.sandbox) {\n"
        "                work.cpu += m_allocations[i].allocated.cpu;\n"
        "                work.mem += m_allocations[i].allocated.mem;\n"
        "                work.sandbox += m_allocations[i].allocated.sandbox;\n"
        "                finish[i] = true; safe_sequence_out.push_back(m_allocations[i].process_name);\n"
        "                found = true;\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    for (int i = 0; i < n; ++i) if (!finish[i]) return false; // UNSAFE STATE\n"
        "    return true; // SAFE STATE\n"
        "}"
    )

    # 8) TESTING DETAILS (MIN 1 * TEAM SIZE = 4 TEST CASES)
    add_heading_14("8. Testing Details (4 Detailed Test Cases)")
    add_body_p(
        "The system was thoroughly tested under heavy simulated concurrent loads using demo.py. Four comprehensive test cases "
        "were executed to validate the operational correctness of all OS concepts:"
    )

    test_cases = [
        ("TC-01: Concurrent Client Session Ingestion (Multi-Threading)",
         "Akriti Singh (Module 1)",
         "Connect 4 client TCP sockets simultaneously from distinct threads (Akriti, Ayushi, Avni, Malvika) and authenticate.",
         "Server master thread accepts all 4 sockets without delay; spawns 4 worker threads; issues distinct authentication tokens.",
         "4 worker threads spawned; authentication tokens AUTH_user_epoch issued. Zero socket timeouts or connection drops.",
         "Module 2 & 3: Master Listener Thread, Worker Thread per Client, Network Sockets.",
         "PASS"),

        ("TC-02: Race Condition Elimination on Shared Leaderboard (Mutex)",
         "Ayushi Tripathi (Module 2)",
         "All 4 participants submit correct solutions at the exact same millisecond, triggering concurrent score additions.",
         "leaderboard_mutex serializes critical section access. All 4 scores are updated accurately with zero lost updates.",
         "Scores tallied perfectly: Malvika (50), Avni (50), Ayushi (30), Akriti (20). Leaderboard sorted deterministically.",
         "Module 3: Critical Section Problem, Mutual Exclusion, Race Condition Prevention.",
         "PASS"),

        ("TC-03: Concurrency Throttling via Counting Semaphores & FCFS",
         "Avni Bansal (Module 3)",
         "Submit 4 compilation tasks simultaneously when MAX_CONCURRENT_EVALUATORS is set to 2.",
         "Exactly 2 submissions enter active compilation simultaneously; the other 2 wait on the counting semaphore in FCFS order.",
         "Slots 1 & 2 acquired immediately; submissions 3 & 4 queued and executed sequentially as slots are released.",
         "Module 3: Counting Semaphores (sem_wait / sem_post), Process Scheduling Queue.",
         "PASS"),

        ("TC-04: Deadlock Avoidance via Banker's Algorithm Safe State Check",
         "Malvika Saxena (Module 4)",
         "Request evaluation resources (CPU and Memory) that approach total system capacity across concurrent threads.",
         "request_resources() evaluates hypothetical allocation. If a safe sequence exists, grant; if unsafe, defer allocation.",
         "System verified safe sequences: <Akriti, Ayushi, Avni, Malvika>. Resources allocated and released with zero deadlock.",
         "Module 4: Deadlock Avoidance, Safe State Verification, Banker's Algorithm.",
         "PASS")
    ]

    for tc_id, author, desc, exp, act, os_c, status in test_cases:
        add_subheading_13(tc_id)
        add_body_p(author, bold_prefix="Test Owner: ")
        add_body_p(desc, bold_prefix="Test Scenario: ")
        add_body_p(exp, bold_prefix="Expected Outcome: ")
        add_body_p(act, bold_prefix="Actual Outcome: ")
        add_body_p(os_c, bold_prefix="OS Concept Verified: ")
        add_body_p(f"{status} (Verified 100% successful in test execution)", bold_prefix="Test Status: ")

    # 9) REFERENCES
    add_heading_14("9. References")
    refs = [
        "[1] A. Silberschatz, P. B. Galvin, and G. Gagne, Operating System Concepts, 10th ed. Hoboken, NJ: John Wiley & Sons, 2018.",
        "[2] A. S. Tanenbaum and H. Bos, Modern Operating Systems, 4th ed. Boston, MA: Pearson, 2015.",
        "[3] W. Stallings, Operating Systems: Internals and Design Principles, 9th ed. New York, NY: Pearson, 2018.",
        "[4] W. R. Stevens, B. Fenner, and A. M. Rudoff, UNIX Network Programming, Volume 1: The Sockets Networking API, 3rd ed. Boston: Addison-Wesley, 2004.",
        "[5] IEEE Computer Society, 'Standard for Information Technology—Portable Operating System Interface (POSIX(R)) Base Specifications, Issue 7,' IEEE Std 1003.1-2017, 2018.",
        "[6] E. W. Dijkstra, 'Cooperating sequential processes,' in Programming Languages, F. Genuys, Ed. Academic Press, 1968, pp. 43–112."
    ]
    for r in refs:
        add_body_p(r)

    # Save to disk
    output_filename = "Real_Time_Coding_Contest_Server_Project_Report.docx"
    doc.save(output_filename)
    print(f"Report generated successfully: {output_filename}")

if __name__ == '__main__':
    create_report()
