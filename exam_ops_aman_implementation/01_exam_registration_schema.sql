-- Exam Operations Platform: Subject, Exam, and Registration module
-- Assumption: a STUDENT table already exists with STUDENT(student_id).
-- If your existing student key has a different name/type, adjust the FK below.
-- MySQL 8.0+ recommended.

CREATE TABLE IF NOT EXISTS SUBJECT (
    subject_id INT AUTO_INCREMENT PRIMARY KEY,
    subject_code VARCHAR(20) NOT NULL UNIQUE,
    subject_name VARCHAR(120) NOT NULL,
    credits TINYINT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS EXAM (
    exam_id INT AUTO_INCREMENT PRIMARY KEY,
    subject_id INT NOT NULL,
    exam_date DATE NOT NULL,
    start_time TIME NOT NULL,
    end_time TIME NOT NULL,
    exam_type VARCHAR(30) NOT NULL DEFAULT 'End-Term',
    exam_status ENUM('SCHEDULED', 'CANCELLED', 'COMPLETED') NOT NULL DEFAULT 'SCHEDULED',
    CONSTRAINT chk_exam_time CHECK (end_time > start_time),
    CONSTRAINT fk_exam_subject
        FOREIGN KEY (subject_id) REFERENCES SUBJECT(subject_id)
        ON UPDATE CASCADE ON DELETE RESTRICT,
    INDEX idx_exam_date (exam_date),
    INDEX idx_exam_subject_id (subject_id),
    INDEX idx_exam_date_subject (exam_date, subject_id)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS REGISTRATION (
    student_id INT NOT NULL,
    exam_id INT NOT NULL,
    registered_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    registration_status ENUM('REGISTERED', 'WITHDRAWN') NOT NULL DEFAULT 'REGISTERED',
    PRIMARY KEY (student_id, exam_id),
    CONSTRAINT fk_registration_student
        FOREIGN KEY (student_id) REFERENCES STUDENT(student_id)
        ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT fk_registration_exam
        FOREIGN KEY (exam_id) REFERENCES EXAM(exam_id)
        ON UPDATE CASCADE ON DELETE CASCADE,
    INDEX idx_registration_exam (exam_id),
    INDEX idx_registration_student_status (student_id, registration_status)
) ENGINE=InnoDB;
