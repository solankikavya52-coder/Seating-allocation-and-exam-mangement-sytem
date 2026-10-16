CREATE DATABASE IF NOT EXISTS exam_management;
USE exam_management;
CREATE TABLE STUDENT (
    student_id INT PRIMARY KEY,
    roll_no VARCHAR(20) UNIQUE NOT NULL,
    full_name VARCHAR(100) NOT NULL
);
CREATE TABLE EXAM (
    exam_id INT PRIMARY KEY,
    exam_name VARCHAR(100) NOT NULL,
    exam_date DATE NOT NULL
);
CREATE TABLE ROOM (
    room_id INT PRIMARY KEY,
    room_name VARCHAR(50) NOT NULL,
    capacity INT NOT NULL CHECK (capacity > 0)
);
CREATE TABLE ALLOCATION (
    allocation_id INT PRIMARY KEY AUTO_INCREMENT,
    exam_id INT NOT NULL,
    student_id INT NOT NULL,
    room_id INT NOT NULL,
    seat_no INT NOT NULL,
    FOREIGN KEY (exam_id)
        REFERENCES EXAM(exam_id),
    FOREIGN KEY (student_id)
        REFERENCES STUDENT(student_id),
    FOREIGN KEY (room_id)
        REFERENCES ROOM(room_id),
    UNIQUE (exam_id, student_id),
    UNIQUE (exam_id, room_id, seat_no)
);
