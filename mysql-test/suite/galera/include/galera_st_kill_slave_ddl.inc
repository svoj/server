#
#  Parameters
#    $conn_node_1= first node connection
#    $conn_node_2= second node connection
#    $conn_node_1_port= first node port
#    $conn_node_2_server_id= second node server id
#

--let $n1= node_1
--let $n2= node_2
--let $n1_port= $NODE_MYPORT_1
--let $n2_server_id= 2
if ($conn_node_1)
{
  --let $n1= $conn_node_1
}
if ($conn_node_2)
{
  --let $n2= $conn_node_2
}
if ($conn_node_1_port)
{
  --let $n1_port= $conn_node_1_port
}
if ($conn_node_2_server_id)
{
  --let $n2_server_id= $conn_node_2_server_id
}
--echo Performing State Transfer on a server that has been killed and restarted
--echo while a DDL was in progress on it

--connection $n1
CREATE TABLE t1 (f1 CHAR(255)) ENGINE=InnoDB;
SET AUTOCOMMIT=OFF;
START TRANSACTION;
INSERT INTO t1 VALUES ('node1_committed_before');
INSERT INTO t1 VALUES ('node1_committed_before');
INSERT INTO t1 VALUES ('node1_committed_before');
INSERT INTO t1 VALUES ('node1_committed_before');
INSERT INTO t1 VALUES ('node1_committed_before');

--connection $n2
START TRANSACTION;
INSERT INTO t1 VALUES ('node2_committed_before');
INSERT INTO t1 VALUES ('node2_committed_before');
INSERT INTO t1 VALUES ('node2_committed_before');
INSERT INTO t1 VALUES ('node2_committed_before');
INSERT INTO t1 VALUES ('node2_committed_before');
COMMIT;

# Suspend the applier as it applies the ALTER TABLE
--let $debug_orig = `SELECT @@debug_dbug`
SET GLOBAL debug_dbug = 'd,sync.alter_opened_table';

--connection $n1
ALTER TABLE t1 ADD COLUMN f2 INTEGER;

--connection $n2
SET wsrep_sync_wait = 0;
--let $wait_condition = SELECT COUNT(*) = 1 FROM INFORMATION_SCHEMA.PROCESSLIST WHERE STATE = 'debug sync point: now'
--source include/wait_condition.inc

--source include/kill_galera.inc

--connection node_1
--let $wait_condition = SELECT VARIABLE_VALUE = 1 FROM INFORMATION_SCHEMA.GLOBAL_STATUS WHERE VARIABLE_NAME = 'wsrep_cluster_size'
--source include/wait_condition.inc

SET AUTOCOMMIT=OFF;
START TRANSACTION;
INSERT INTO t1 (f1) VALUES ('node1_committed_during');
INSERT INTO t1 (f1) VALUES ('node1_committed_during');
INSERT INTO t1 (f1) VALUES ('node1_committed_during');
INSERT INTO t1 (f1) VALUES ('node1_committed_during');
INSERT INTO t1 (f1) VALUES ('node1_committed_during');
COMMIT;

START TRANSACTION;
INSERT INTO t1 (f1) VALUES ('node1_to_be_committed_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_committed_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_committed_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_committed_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_committed_after');

--connect node_1a_galera_st_kill_slave_ddl, 127.0.0.1, root, , test, $n1_port
SET AUTOCOMMIT=OFF;
START TRANSACTION;
INSERT INTO t1 (f1) VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_rollbacked_after');

--connection $n2
--let $galera_wsrep_recover_server_id=$n2_server_id
--source suite/galera/include/galera_wsrep_recover.inc

--connection $n2
--echo Starting server ...
--source include/start_mysqld.inc
--source include/wait_until_ready.inc

SET AUTOCOMMIT=OFF;
START TRANSACTION;
INSERT INTO t1 (f1) VALUES ('node2_committed_after');
INSERT INTO t1 (f1) VALUES ('node2_committed_after');
INSERT INTO t1 (f1) VALUES ('node2_committed_after');
INSERT INTO t1 (f1) VALUES ('node2_committed_after');
INSERT INTO t1 (f1) VALUES ('node2_committed_after');
COMMIT;

--connection $n1
INSERT INTO t1 (f1) VALUES ('node1_to_be_committed_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_committed_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_committed_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_committed_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_committed_after');
COMMIT;

SET AUTOCOMMIT=OFF;
START TRANSACTION;
INSERT INTO t1 (f1) VALUES ('node1_committed_after');
INSERT INTO t1 (f1) VALUES ('node1_committed_after');
INSERT INTO t1 (f1) VALUES ('node1_committed_after');
INSERT INTO t1 (f1) VALUES ('node1_committed_after');
INSERT INTO t1 (f1) VALUES ('node1_committed_after');
COMMIT;

--connection node_1a_galera_st_kill_slave_ddl
INSERT INTO t1 (f1) VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 (f1) VALUES ('node1_to_be_rollbacked_after');
ROLLBACK;

SELECT COUNT(*) = 2 FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = 't1';
SELECT COUNT(*) = 35 FROM t1;
SELECT COUNT(*) = 0 FROM (SELECT COUNT(*) AS c, f1 FROM t1 GROUP BY f1 HAVING c NOT IN (5, 10)) AS a1;
COMMIT;
SET AUTOCOMMIT=ON;

--connection $n1
SELECT COUNT(*) = 2 FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = 't1';
SELECT COUNT(*) = 35 FROM t1;
SELECT COUNT(*) = 0 FROM (SELECT COUNT(*) AS c, f1 FROM t1 GROUP BY f1 HAVING c NOT IN (5, 10)) AS a1;
DROP TABLE t1;
COMMIT;
SET AUTOCOMMIT=ON;

SET GLOBAL debug_dbug = $debug_orig;
