CREATE DATABASE IF NOT EXISTS exam_system;

USE exam_system;

CREATE TABLE IF NOT EXISTS BRANCH (
    branch_id INT AUTO_INCREMENT PRIMARY KEY,
    branch_name VARCHAR(100) NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS SEMESTER (
    semester_id INT AUTO_INCREMENT PRIMARY KEY,
    semester_no INT NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS USER (
    user_id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    role ENUM('Admin', 'Invigilator', 'Student') NOT NULL
);

CREATE TABLE IF NOT EXISTS STUDENT (
    student_id INT AUTO_INCREMENT PRIMARY KEY,
    roll_no VARCHAR(30) NOT NULL UNIQUE,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(100) UNIQUE,
    branch_id INT NOT NULL,
    semester_id INT NOT NULL,
    FOREIGN KEY (branch_id) REFERENCES BRANCH(branch_id),
    FOREIGN KEY (semester_id) REFERENCES SEMESTER(semester_id)
);

CREATE TABLE IF NOT EXISTS ROOM (
    room_id INT AUTO_INCREMENT PRIMARY KEY,
    room_no VARCHAR(20) NOT NULL UNIQUE,
    capacity INT NOT NULL CHECK (capacity > 0)
);

CREATE TABLE IF NOT EXISTS ALLOCATION (
    allocation_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    student_id INT NOT NULL UNIQUE,
    room_id INT NOT NULL,
    seat_no INT NOT NULL,
    allocated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_allocation_student FOREIGN KEY (student_id)
        REFERENCES STUDENT(student_id) ON DELETE CASCADE,
    CONSTRAINT fk_allocation_room FOREIGN KEY (room_id)
        REFERENCES ROOM(room_id) ON DELETE CASCADE,
    CONSTRAINT uq_student_allocation UNIQUE (student_id),
    CONSTRAINT uq_room_seat UNIQUE (room_id, seat_no),
    INDEX idx_allocation_student (student_id),
    INDEX idx_allocation_room (room_id)
) ENGINE=InnoDB;

CREATE INDEX idx_student_branch
ON STUDENT(branch_id);

CREATE INDEX idx_student_name
ON STUDENT(name);

CREATE INDEX idx_room_capacity
ON ROOM(capacity);