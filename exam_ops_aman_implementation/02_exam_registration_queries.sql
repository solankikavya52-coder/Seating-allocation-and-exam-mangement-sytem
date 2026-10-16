-- Run after 01_exam_registration_schema.sql.
-- Adjust STUDENT column names if your existing schema differs.

-- 1. Student-wise exam timetable
SELECT
    s.student_id,
    s.name AS student_name,
    e.exam_id,
    sub.subject_code,
    sub.subject_name,
    e.exam_date,
    e.start_time,
    e.end_time,
    e.exam_type,
    e.exam_status
FROM STUDENT AS s
JOIN REGISTRATION AS reg ON reg.student_id = s.student_id
JOIN EXAM AS e ON e.exam_id = reg.exam_id
JOIN SUBJECT AS sub ON sub.subject_id = e.subject_id
WHERE reg.registration_status = 'REGISTERED'
ORDER BY s.student_id, e.exam_date, e.start_time;

-- 2. All registered students for one exam (replace 1 with an exam_id)
SELECT
    e.exam_id,
    sub.subject_code,
    sub.subject_name,
    s.student_id,
    s.name AS student_name,
    reg.registered_at
FROM EXAM AS e
JOIN SUBJECT AS sub ON sub.subject_id = e.subject_id
JOIN REGISTRATION AS reg ON reg.exam_id = e.exam_id
JOIN STUDENT AS s ON s.student_id = reg.student_id
WHERE e.exam_id = 1
  AND reg.registration_status = 'REGISTERED'
ORDER BY s.student_id;

-- 3. Exam-wise registration count
SELECT
    e.exam_id,
    sub.subject_code,
    sub.subject_name,
    e.exam_date,
    COUNT(reg.student_id) AS registered_students
FROM EXAM AS e
JOIN SUBJECT AS sub ON sub.subject_id = e.subject_id
LEFT JOIN REGISTRATION AS reg
    ON reg.exam_id = e.exam_id
   AND reg.registration_status = 'REGISTERED'
GROUP BY e.exam_id, sub.subject_code, sub.subject_name, e.exam_date
ORDER BY e.exam_date, e.exam_id;

-- 4. Find exams for a subject (replace 1 with subject_id)
SELECT e.exam_id, sub.subject_code, sub.subject_name,
       e.exam_date, e.start_time, e.end_time, e.exam_status
FROM EXAM AS e
JOIN SUBJECT AS sub ON sub.subject_id = e.subject_id
WHERE e.subject_id = 1
ORDER BY e.exam_date, e.start_time;

-- 5. Find registered exams for one student (replace 101 with student_id)
SELECT e.exam_id, sub.subject_name, e.exam_date, e.start_time, e.end_time
FROM REGISTRATION AS reg
JOIN EXAM AS e ON e.exam_id = reg.exam_id
JOIN SUBJECT AS sub ON sub.subject_id = e.subject_id
WHERE reg.student_id = 101
  AND reg.registration_status = 'REGISTERED'
ORDER BY e.exam_date, e.start_time;

-- 6. Students registered for overlapping exam slots (basic same-date overlap check)
SELECT
    r1.student_id,
    e1.exam_id AS first_exam_id,
    e2.exam_id AS second_exam_id,
    e1.exam_date
FROM REGISTRATION AS r1
JOIN REGISTRATION AS r2
  ON r2.student_id = r1.student_id
 AND r2.exam_id > r1.exam_id
JOIN EXAM AS e1 ON e1.exam_id = r1.exam_id
JOIN EXAM AS e2 ON e2.exam_id = r2.exam_id
WHERE r1.registration_status = 'REGISTERED'
  AND r2.registration_status = 'REGISTERED'
  AND e1.exam_date = e2.exam_date
  AND e1.start_time < e2.end_time
  AND e2.start_time < e1.end_time;

-- 7. Optional view for a reusable student timetable
CREATE OR REPLACE VIEW vw_student_exam_timetable AS
SELECT
    s.student_id,
    s.name AS student_name,
    e.exam_id,
    sub.subject_code,
    sub.subject_name,
    e.exam_date,
    e.start_time,
    e.end_time,
    e.exam_type,
    e.exam_status,
    reg.registration_status
FROM STUDENT AS s
JOIN REGISTRATION AS reg ON reg.student_id = s.student_id
JOIN EXAM AS e ON e.exam_id = reg.exam_id
JOIN SUBJECT AS sub ON sub.subject_id = e.subject_id;
