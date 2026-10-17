USE exam_system;
INSERT INTO ROOM_PROFILE(room_id, building_name, floor_number, room_type, room_status, has_accessible_seating, notes)
SELECT room_id, 'Main Campus', 0, 'CLASSROOM', 'AVAILABLE', FALSE, 'Update this profile with the actual campus room details'
FROM ROOM
ON DUPLICATE KEY UPDATE room_id=VALUES(room_id);
