# Exam Operations & Seating Allocation — Aman’s Module

## Included files
- `01_exam_registration_schema.sql`: creates `SUBJECT`, `EXAM`, and `REGISTRATION`.
- `02_exam_registration_queries.sql`: join-heavy timetable, registration, count, overlap query, and view.
- `03_cpu_scheduler.cpp`: standalone FCFS and Round Robin job-queue simulation with waiting/turnaround metrics.

## Important integration assumption
The SQL assumes your team's existing `STUDENT` table has an integer primary key named `student_id`, and a text column named `name` for the queries. If the team's actual names/types differ, update the foreign key and `s.name` references before running. Run this against the team's existing database; do not create a second conflicting STUDENT table.

## Run the SQL
1. Select the project database, for example: `USE exam_system;`
2. Run `01_exam_registration_schema.sql`.
3. Run `02_exam_registration_queries.sql`.
4. Insert sample `SUBJECT` and `EXAM` rows, and ensure the referenced students already exist before inserting `REGISTRATION` rows.
5. The composite primary key `(student_id, exam_id)` blocks duplicate registration for the same student and exam.

## Compile and run the OS scheduler (C++17)
Linux / Ubuntu:
```bash
g++ -std=c++17 -Wall -Wextra -O2 03_cpu_scheduler.cpp -o cpu_scheduler
./cpu_scheduler
```

Windows with MinGW:
```bash
g++ -std=c++17 -Wall -Wextra -O2 03_cpu_scheduler.cpp -o cpu_scheduler.exe
cpu_scheduler.exe
```

Input format:
- First enter the number of jobs.
- Then enter one row per job: `Exam_ID Arrival_Time Burst_Time`
- Finally enter a positive Round Robin time quantum.

Example:
```text
4
101 0 8
102 1 4
103 2 9
104 3 5
3
```

## Scheduling metrics
- Turnaround time = Completion time - Arrival time
- Waiting time = Turnaround time - Burst time
- FCFS processes jobs in arrival order.
- Round Robin gives each ready job at most `quantum` time units per turn.

This is a scheduling-policy simulation. It does not launch operating-system processes or execute database allocation jobs itself. In the project report, describe burst times as estimated processing durations. If you later connect the scheduler to real jobs, log actual enqueue/start/finish timestamps and compare them with these simulation estimates.

## Design notes
- `EXAM.subject_id` is indexed and `EXAM.exam_date` is indexed; a combined `(exam_date, subject_id)` index supports common date-plus-subject filters.
- InnoDB foreign keys and the `REGISTRATION` composite primary key enforce referential integrity and duplicate-registration prevention.
- `CHECK (end_time > start_time)` requires a MySQL version that enforces CHECK constraints (MySQL 8.0.16+).
- The scheduling queue is in memory for this deliverable. A database-backed persistent queue can be added if the team requires jobs to survive application restarts.
