-- Student 5 module for the team's exam_system database.
-- Requires the shared EXAM table from Student 2's Exam & Registration module.
USE exam_system;
CREATE TABLE IF NOT EXISTS INVIGILATOR (
  invigilator_id INT NOT NULL AUTO_INCREMENT,
  employee_code VARCHAR(30) NOT NULL,
  full_name VARCHAR(120) NOT NULL,
  email VARCHAR(160) NOT NULL,
  department VARCHAR(100) NOT NULL,
  phone VARCHAR(25) NULL,
  is_active BOOLEAN NOT NULL DEFAULT TRUE,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (invigilator_id), UNIQUE KEY uq_invigilator_code(employee_code),
  UNIQUE KEY uq_invigilator_email(email), KEY idx_invigilator_active_department(is_active,department)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS EXAM_INVIGILATOR (
  duty_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  exam_id INT NOT NULL,
  room_id INT NOT NULL,
  invigilator_id INT NOT NULL,
  duty_role ENUM('CHIEF_INVIGILATOR','ASSISTANT_INVIGILATOR','RELIEF_INVIGILATOR') NOT NULL DEFAULT 'ASSISTANT_INVIGILATOR',
  duty_status ENUM('ASSIGNED','CONFIRMED','CANCELLED') NOT NULL DEFAULT 'ASSIGNED',
  assigned_by INT NULL,
  assigned_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  notes VARCHAR(300) NULL,
  PRIMARY KEY (duty_id), UNIQUE KEY uq_exam_room_invigilator(exam_id,room_id,invigilator_id),
  KEY idx_duty_invigilator_status(invigilator_id,duty_status), KEY idx_duty_exam_room(exam_id,room_id),
  CONSTRAINT fk_duty_exam FOREIGN KEY (exam_id) REFERENCES EXAM(exam_id),
  CONSTRAINT fk_duty_room FOREIGN KEY (room_id) REFERENCES ROOM(room_id),
  CONSTRAINT fk_duty_invigilator FOREIGN KEY (invigilator_id) REFERENCES INVIGILATOR(invigilator_id)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS ATTENDANCE_SESSION (
  session_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  exam_id INT NOT NULL,
  room_id INT NOT NULL,
  opened_by INT NOT NULL,
  session_status ENUM('OPEN','SUBMITTED','REOPENED') NOT NULL DEFAULT 'OPEN',
  opened_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  submitted_at DATETIME NULL,
  notes VARCHAR(300) NULL,
  PRIMARY KEY (session_id), UNIQUE KEY uq_attendance_session_exam_room(exam_id,room_id),
  KEY idx_attendance_session_status(session_status,opened_at),
  CONSTRAINT fk_attendance_session_exam FOREIGN KEY (exam_id) REFERENCES EXAM(exam_id),
  CONSTRAINT fk_attendance_session_room FOREIGN KEY (room_id) REFERENCES ROOM(room_id),
  CONSTRAINT fk_attendance_session_invigilator FOREIGN KEY (opened_by) REFERENCES INVIGILATOR(invigilator_id)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS ATTENDANCE (
  attendance_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  session_id BIGINT UNSIGNED NOT NULL,
  student_id INT NOT NULL,
  attendance_status ENUM('PRESENT','ABSENT','LATE','EXCUSED') NOT NULL,
  marked_by INT NOT NULL,
  marked_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  remarks VARCHAR(300) NULL,
  PRIMARY KEY (attendance_id), UNIQUE KEY uq_attendance_student_session(session_id,student_id),
  KEY idx_attendance_status_time(attendance_status,marked_at), KEY idx_attendance_student(student_id,session_id),
  CONSTRAINT fk_attendance_session FOREIGN KEY (session_id) REFERENCES ATTENDANCE_SESSION(session_id),
  CONSTRAINT fk_attendance_student FOREIGN KEY (student_id) REFERENCES STUDENT(student_id),
  CONSTRAINT fk_attendance_marker FOREIGN KEY (marked_by) REFERENCES INVIGILATOR(invigilator_id)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS ATTENDANCE_CHANGE_LOG (
  change_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  attendance_id BIGINT UNSIGNED NOT NULL,
  old_status ENUM('PRESENT','ABSENT','LATE','EXCUSED') NOT NULL,
  new_status ENUM('PRESENT','ABSENT','LATE','EXCUSED') NOT NULL,
  changed_by INT NOT NULL,
  change_reason VARCHAR(300) NOT NULL,
  changed_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (change_id), KEY idx_attendance_change_record_time(attendance_id,changed_at),
  CONSTRAINT fk_attendance_change_record FOREIGN KEY (attendance_id) REFERENCES ATTENDANCE(attendance_id),
  CONSTRAINT fk_attendance_change_invigilator FOREIGN KEY (changed_by) REFERENCES INVIGILATOR(invigilator_id)
) ENGINE=InnoDB;

CREATE OR REPLACE VIEW VW_EXAM_ATTENDANCE_SUMMARY AS
SELECT s.session_id,s.exam_id,s.room_id,r.room_no,
 COUNT(a.attendance_id) AS marked_students,
 COALESCE(SUM(a.attendance_status='PRESENT'),0) AS present_count,
 COALESCE(SUM(a.attendance_status='ABSENT'),0) AS absent_count,
 COALESCE(SUM(a.attendance_status='LATE'),0) AS late_count,
 COALESCE(SUM(a.attendance_status='EXCUSED'),0) AS excused_count,
 s.session_status,s.opened_at,s.submitted_at
FROM ATTENDANCE_SESSION s JOIN ROOM r ON r.room_id=s.room_id
LEFT JOIN ATTENDANCE a ON a.session_id=s.session_id
GROUP BY s.session_id,s.exam_id,s.room_id,r.room_no,s.session_status,s.opened_at,s.submitted_at;

CREATE OR REPLACE VIEW VW_INVIGILATOR_DUTY_SCHEDULE AS
SELECT i.invigilator_id,i.employee_code,i.full_name,i.department,d.exam_id,d.room_id,
 r.room_no,d.duty_role,d.duty_status,d.assigned_at
FROM INVIGILATOR i JOIN EXAM_INVIGILATOR d ON d.invigilator_id=i.invigilator_id
JOIN ROOM r ON r.room_id=d.room_id;
