-- =============================================================================
-- Module: Seating Allocation Queries & Analytical Views
-- Author: Ayush Chauhan (Student 4, Roll No: 2418385)
-- =============================================================================

USE exam_system;

-- 1. Room-Wise Seating Plan View
-- Maps allocated students to physical room numbers, seats, branches, and semesters.
CREATE OR REPLACE VIEW vw_room_seating_plan AS
SELECT 
    r.room_id,
    r.room_no,
    a.seat_no,
    s.roll_no,
    s.name AS student_name,
    b.branch_name,
    se.semester_no,
    a.allocated_at
FROM ALLOCATION a
JOIN STUDENT s ON a.student_id = s.student_id
JOIN ROOM r ON a.room_id = r.room_id
JOIN BRANCH b ON s.branch_id = b.branch_id
JOIN SEMESTER se ON s.semester_id = se.semester_id
ORDER BY r.room_no ASC, a.seat_no ASC;

-- 2. Fast Roll Number Lookup Query
-- Parameterized query used by students to check assigned room and seat.
SELECT 
    s.roll_no,
    s.name AS student_name,
    r.room_no,
    a.seat_no,
    a.allocated_at
FROM ALLOCATION a
JOIN STUDENT s ON a.student_id = s.student_id
JOIN ROOM r ON a.room_id = r.room_id
WHERE s.roll_no = '2418385';

-- 3. Room Occupancy & Capacity Utilization View
-- Computes real-time occupancy counts, vacant seats, and occupancy percentages per room.
CREATE OR REPLACE VIEW vw_room_occupancy_status AS
SELECT 
    r.room_id,
    r.room_no,
    r.capacity AS total_capacity,
    COUNT(a.allocation_id) AS occupied_seats,
    (r.capacity - COUNT(a.allocation_id)) AS vacant_seats,
    ROUND((COUNT(a.allocation_id) / r.capacity) * 100, 2) AS occupancy_percent
FROM ROOM r
LEFT JOIN ALLOCATION a ON r.room_id = a.room_id
GROUP BY r.room_id, r.room_no, r.capacity
ORDER BY r.room_no ASC;
