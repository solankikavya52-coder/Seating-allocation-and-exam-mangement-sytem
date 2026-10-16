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
SELECT
    e.exam_name,
    r.room_name,
    r.capacity,
    COUNT(a.allocation_id) AS occupied_seats,
    r.capacity - COUNT(a.allocation_id) AS available_seats
FROM EXAM e
JOIN ALLOCATION a
    ON e.exam_id = a.exam_id
JOIN ROOM r
    ON a.room_id = r.room_id
GROUP BY
    e.exam_id,
    e.exam_name,
    r.room_id,
    r.room_name,
    r.capacity
ORDER BY e.exam_name, r.room_name;
