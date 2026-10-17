USE exam_system;
CREATE TABLE IF NOT EXISTS ROOM_PROFILE (
  room_id INT NOT NULL,
  building_name VARCHAR(100) NOT NULL DEFAULT 'Main Campus',
  floor_number SMALLINT NOT NULL DEFAULT 0,
  room_type ENUM('EXAM_HALL','CLASSROOM','LABORATORY','AUDITORIUM') NOT NULL DEFAULT 'CLASSROOM',
  room_status ENUM('AVAILABLE','UNAVAILABLE','UNDER_MAINTENANCE') NOT NULL DEFAULT 'AVAILABLE',
  has_accessible_seating BOOLEAN NOT NULL DEFAULT FALSE,
  notes VARCHAR(500) NULL,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (room_id),
  CONSTRAINT fk_room_profile_room FOREIGN KEY (room_id) REFERENCES ROOM(room_id) ON DELETE CASCADE
) ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS ROOM_SEAT (
  room_seat_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  room_id INT NOT NULL,
  seat_no INT NOT NULL,
  row_label VARCHAR(10) NOT NULL,
  seat_label VARCHAR(20) NOT NULL,
  seat_status ENUM('AVAILABLE','ALLOCATED','BLOCKED') NOT NULL DEFAULT 'AVAILABLE',
  is_accessible BOOLEAN NOT NULL DEFAULT FALSE,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (room_seat_id),
  UNIQUE KEY uq_room_seat_number (room_id, seat_no),
  UNIQUE KEY uq_room_seat_label (room_id, seat_label),
  KEY idx_room_seat_status (room_id, seat_status),
  CONSTRAINT fk_room_seat_room FOREIGN KEY (room_id) REFERENCES ROOM(room_id) ON DELETE CASCADE,
  CONSTRAINT chk_room_seat_no_positive CHECK (seat_no > 0)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS ROOM_STATUS_HISTORY (
  history_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  room_id INT NOT NULL,
  old_status ENUM('AVAILABLE','UNAVAILABLE','UNDER_MAINTENANCE') NULL,
  new_status ENUM('AVAILABLE','UNAVAILABLE','UNDER_MAINTENANCE') NOT NULL,
  reason VARCHAR(300) NOT NULL,
  changed_by INT NULL,
  changed_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (history_id),
  KEY idx_room_history_time (room_id, changed_at),
  CONSTRAINT fk_room_status_history_room FOREIGN KEY (room_id) REFERENCES ROOM(room_id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS ROOM_MAINTENANCE (
  maintenance_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  room_id INT NOT NULL,
  starts_at DATETIME NOT NULL,
  ends_at DATETIME NOT NULL,
  reason VARCHAR(300) NOT NULL,
  recorded_by INT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (maintenance_id),
  KEY idx_room_maintenance_window (room_id, starts_at, ends_at),
  CONSTRAINT fk_room_maintenance_room FOREIGN KEY (room_id) REFERENCES ROOM(room_id) ON DELETE CASCADE,
  CONSTRAINT chk_room_maintenance_window CHECK (ends_at > starts_at)
) ENGINE=InnoDB;

-- This procedure creates the physical seat inventory from ROOM.capacity.
DELIMITER $$
CREATE PROCEDURE sp_generate_room_seats(IN p_room_id INT)
BEGIN
  DECLARE v_capacity INT;
  DECLARE v_existing INT;
  DECLARE v_n INT DEFAULT 1;
  DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
  START TRANSACTION;
  SELECT capacity INTO v_capacity FROM ROOM WHERE room_id=p_room_id FOR UPDATE;
  IF v_capacity IS NULL THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Room was not found'; END IF;
  SELECT COUNT(*) INTO v_existing FROM ROOM_SEAT WHERE room_id=p_room_id;
  IF v_existing > v_capacity THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Existing seat inventory exceeds room capacity'; END IF;
  WHILE v_n <= v_capacity DO
    INSERT IGNORE INTO ROOM_SEAT(room_id, seat_no, row_label, seat_label, seat_status)
    VALUES(p_room_id, v_n, CONCAT(CHAR(65 + FLOOR((v_n - 1) / 10)), ''), CONCAT(CHAR(65 + FLOOR((v_n - 1) / 10)), LPAD(MOD(v_n - 1, 10) + 1, 2, '0')), 'AVAILABLE');
    SET v_n = v_n + 1;
  END WHILE;
  COMMIT;
END$$

CREATE PROCEDURE sp_set_room_status(
  IN p_room_id INT,
  IN p_new_status VARCHAR(30),
  IN p_reason VARCHAR(300),
  IN p_changed_by INT
)
BEGIN
  DECLARE v_old_status VARCHAR(30);
  DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
  IF p_new_status NOT IN ('AVAILABLE','UNAVAILABLE','UNDER_MAINTENANCE') THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Invalid room status';
  END IF;
  IF p_reason IS NULL OR CHAR_LENGTH(TRIM(p_reason)) < 3 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Please provide a clear reason for the room status change';
  END IF;
  START TRANSACTION;
  SELECT room_status INTO v_old_status FROM ROOM_PROFILE WHERE room_id=p_room_id FOR UPDATE;
  IF v_old_status IS NULL THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Room profile was not found'; END IF;
  UPDATE ROOM_PROFILE SET room_status=p_new_status WHERE room_id=p_room_id;
  INSERT INTO ROOM_STATUS_HISTORY(room_id, old_status, new_status, reason, changed_by)
  VALUES(p_room_id, v_old_status, p_new_status, TRIM(p_reason), p_changed_by);
  COMMIT;
END$$
DELIMITER ;
