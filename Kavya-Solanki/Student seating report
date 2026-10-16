SELECT
    s.roll_no,
    s.full_name,
    e.exam_name,
    e.exam_date,
    r.room_name,
    a.seat_no
FROM ALLOCATION a
JOIN STUDENT s
    ON a.student_id = s.student_id
JOIN EXAM e
    ON a.exam_id = e.exam_id
JOIN ROOM r
    ON a.room_id = r.room_id
ORDER BY e.exam_date, r.room_name, a.seat_no;
