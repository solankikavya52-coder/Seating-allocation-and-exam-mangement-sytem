# Seating Allocation Engine (DBMS + Operating Systems)
**Author:** Ayush Chauhan (Student 4, Roll No: 2418385)  
**Assigned Module:** Seating Allocation Module (5th Semester PBL)  

---

## 1. Overview & Files
This directory contains the complete implementation for the **Seating Allocation Engine**:

| File | Domain | Description |
|:---|:---|:---|
| [`allocation.cpp`](allocation.cpp) | **DBMS** | Production transactional C++ seating allocation engine with MySQL ACID boundaries (atomic commit/rollback) and capacity validation. |
| [`concurrency_engine.cpp`](concurrency_engine.cpp) | **OS** | Multithreaded simulation demonstrating race conditions on seat assignment vs synchronized allocation using `std::mutex`. |
| [`allocation_queries.sql`](allocation_queries.sql) | **DBMS** | SQL views for room-wise seating charts (`vw_room_seating_plan`), student roll-number lookup, and room occupancy status (`vw_room_occupancy_status`). |

---

## 2. Technical Implementation

### DBMS Core (ACID & Constraints)
- **Atomicity:** The entire allocation is wrapped in a transaction block (`con->setAutoCommit(false)`). If any insert fails, `con->rollback()` executes, guaranteeing zero partial writes.
- **Constraints:** Enforces `UNIQUE(room_id, seat_no)` so no seat can ever be double-booked, and `UNIQUE(student_id)` so no student can be in two rooms at once.
- **Capacity Validation:** Pre-validates that total available seats across all rooms $\ge$ total students before modifying any database state.

### Operating Systems Core (Multithreading & Mutual Exclusion)
- **Workload Partitioning:** Distributes student allocation across 4 concurrent worker threads using `std::thread`.
- **Critical Section:** Isolates the seat check-and-claim step (`allocatedCount < capacity`).
- **Race Condition vs Mutex:**
  - **Mode 1 (`--race`):** Without locks, threads interleave on seat counters, causing duplicate seat assignments (collisions).
  - **Mode 2 (`--safe`):** Fine-grained per-room locks (`std::mutex`, `std::unique_lock`) prevent race conditions, achieving 100% thread safety with 0 collisions.

---

## 3. How to Compile and Run

### Running the OS Concurrency Simulation (No external dependencies)
```bash
# Compile:
g++ -std=c++17 -O2 concurrency_engine.cpp -o concurrency_engine -pthread

# Run comparative benchmark (Race vs Mutex):
./concurrency_engine --compare

# Run only race condition demonstration:
./concurrency_engine --race

# Run only mutex-protected safe mode:
./concurrency_engine --safe
```

### Running the Transactional MySQL Allocation Engine
Prerequisite: Running MySQL Server with `exam_system` database and tables loaded.
```bash
g++ -std=c++17 allocation.cpp -o allocation.exe -lmysqlcppconn
./allocation.exe
```

---

## 4. Viva Pitch
> *"I am Student 4, Ayush Chauhan (Roll No: 2418385), responsible for the **Seating Allocation Engine**.  
> On the **DBMS side**, I implemented the transactional allocation engine with strict ACID rollback guarantees, capacity pre-validation, and `UNIQUE(room_id, seat_no)` constraints to eliminate double-booking.  
> On the **OS side**, I built the multithreaded allocation pipeline, identified the seat read-and-claim critical section, and proved mutual exclusion by demonstrating race condition failures without locks versus 100% synchronized allocations using mutexes."*
