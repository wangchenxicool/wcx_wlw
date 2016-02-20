BEGIN TRANSACTION;
CREATE TABLE USER(user_name varchar(20),passwd varchar(20),rtu_sn integer);
INSERT INTO "USER" VALUES('0001','123',1);
COMMIT;
