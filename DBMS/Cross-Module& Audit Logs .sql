--cross-module view
CREATE OR REPLACE VIEW vw_seating_report AS
SELECT
    s.student_id,
    s.student_name,
    e.exam_id,
    e.exam_name,
    e.exam_date,
    r.room_id,
    r.room_name,
    a.seat_number
FROM STUDENT s
JOIN ALLOCATION a
    ON s.student_id = a.student_id
JOIN EXAM e
    ON a.exam_id = e.exam_id
JOIN ROOM r
    ON a.room_id = r.room_id;
--for viewing
SELECT * FROM vw_seating_report;
--audit log
CREATE TABLE IF NOT EXISTS AUDIT_LOG (
    log_id INT AUTO_INCREMENT PRIMARY KEY,
    action_type VARCHAR(50) NOT NULL,
    student_id INT,
    exam_id INT,
    old_room_id INT,
    new_room_id INT,
    action_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    description VARCHAR(255)
);
--view auditt
SELECT *
FROM AUDIT_LOG
ORDER BY action_time DESC;
