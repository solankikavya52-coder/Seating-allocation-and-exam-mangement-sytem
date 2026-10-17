USE exam_system;
INSERT INTO INVIGILATOR(employee_code,full_name,email,department,phone) VALUES
 ('INV-1001','Dr. Meera Sharma','meera.sharma@example.edu','Computer Science','9000000101'),
 ('INV-1002','Mr. Rohan Verma','rohan.verma@example.edu','Mathematics','9000000102'),
 ('INV-1003','Ms. Nisha Kapoor','nisha.kapoor@example.edu','Electronics','9000000103')
ON DUPLICATE KEY UPDATE full_name=VALUES(full_name),department=VALUES(department);

-- Once Student 2's EXAM table contains the exam and ROOM contains the room, assign duty like this:
-- INSERT INTO EXAM_INVIGILATOR(exam_id,room_id,invigilator_id,duty_role)
-- SELECT 1,1,invigilator_id,'CHIEF_INVIGILATOR' FROM INVIGILATOR WHERE employee_code='INV-1001';
