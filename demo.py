"""
================================================================================
Demonstration & Concurrency Simulation Script
Project: Real-Time Coding Contest Server (OS Lab 25B16CS211)
Team: Akriti Singh, Ayushi Tripathi, Avni Bansal, Malvika Saxena
================================================================================
This script automates the execution of:
1. Launching the C++ Contest Server
2. Simulating 4 concurrent participants connecting simultaneously
3. Demonstrating:
   - Concurrency & Threading (Module 3)
   - Mutex Synchronization & Critical Section Protection (Module 3)
   - Counting Semaphore Evaluator Limiting (Module 3)
   - Deadlock Avoidance with Banker's Algorithm (Module 4)
   - Real-time Leaderboard Output
================================================================================
"""

import subprocess
import socket
import time
import threading
import sys
import os

SERVER_HOST = '127.0.0.1'
SERVER_PORT = 8080

def participant_session(name, submissions):
    """Simulates one student participant connecting over TCP and submitting solutions."""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((SERVER_HOST, SERVER_PORT))
        f = s.makefile('r')

        # Read banner
        banner = f.readline().strip()
        print(f"[{name}] Connected -> {banner}")

        # Login
        s.sendall(f"LOGIN {name}\n".encode())
        resp = f.readline().strip()
        print(f"[{name}] {resp}")

        # Perform submissions
        for prob_id, code in submissions:
            time.sleep(0.2) # Small realistic delay
            print(f"[{name}] >>> Submitting Problem {prob_id}...")
            s.sendall(f"SUBMIT {prob_id} {code}\n".encode())
            
            queue_msg = f.readline().strip()
            print(f"[{name}] {queue_msg}")
            
            result_msg = f.readline().strip()
            print(f"[{name}] {result_msg}")

        # Check status
        s.sendall(b"STATUS\n")
        status = f.readline().strip()
        print(f"[{name}] Status: {status}")

        s.sendall(b"EXIT\n")
        f.close()
        s.close()
    except Exception as e:
        print(f"[{name}] Error: {e}")

def main():
    print("=" * 70)
    print("  STARTING CONCURRENCY & OS SIMULATION TEST")
    print("  Real-Time Coding Contest Server")
    print("=" * 70)

    # 1. Start Server Process
    exe_name = "server.exe" if os.name == 'nt' else "./server"
    if not os.path.exists(exe_name.replace("./", "")):
        print(f"Error: {exe_name} not found. Please compile first.")
        sys.exit(1)

    print(f"[*] Launching {exe_name} in background...")
    server_proc = subprocess.Popen([exe_name], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
    
    # Background thread to print server logs in real time
    def stream_server_logs():
        for line in server_proc.stdout:
            print(f"    [SERVER LOG] {line.strip()}")

    log_thread = threading.Thread(target=stream_server_logs, daemon=True)
    log_thread.start()

    time.sleep(1.5) # Wait for server socket to initialize

    # 2. Define the 4 group members with their submissions
    team_members = [
        ("Akriti_Singh",     [(1, "std::reverse(arr.begin(), arr.end());")]),
        ("Ayushi_Tripathi",  [(2, "sem_wait(&empty); pthread_mutex_lock(&mtx);")]),
        ("Avni_Bansal",      [(1, "for(int i=n-1; i>=0; --i) swap();"), (2, "sem_post(&full);")]),
        ("Malvika_Saxena",   [(3, "bankers_is_safe(alloc, max, avail);")]),
    ]

    print("\n[*] Spawning 4 concurrent student threads to simulate real contest load...\n")
    threads = []
    for name, subs in team_members:
        t = threading.Thread(target=participant_session, args=(name, subs))
        threads.append(t)
        t.start()

    for t in threads:
        t.join()

    # 3. Retrieve Final Live Leaderboard
    time.sleep(1)
    print("\n" + "=" * 70)
    print("  FETCHING FINAL SYNCHRONIZED LEADERBOARD")
    print("=" * 70)
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((SERVER_HOST, SERVER_PORT))
        f = s.makefile('r')
        f.readline() # Banner
        s.sendall(b"LOGIN Admin_Viewer\n")
        f.readline()
        s.sendall(b"LEADERBOARD\n")
        lines_seen = 0
        while True:
            line = f.readline()
            if not line: break
            print(line.rstrip())
            if "=========================================================================" in line:
                lines_seen += 1
                if lines_seen >= 3:
                    break
        s.sendall(b"EXIT\n")
        f.close()
        s.close()
    except Exception as e:
        print(f"Error fetching leaderboard: {e}")

    # Terminate server
    print("\n[*] Demonstration complete. Shutting down server...")
    server_proc.terminate()
    try:
        server_proc.wait(timeout=2)
    except:
        server_proc.kill()

    print("[*] Simulation successfully finished.")

if __name__ == '__main__':
    main()
