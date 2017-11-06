#
#  Parameters
#    $conn_node_1= first node connection
#    $conn_node_2= second node connection
#    $conn_node_1_port= first node port
#

--let $n1= node_1
--let $n2= node_2
--let $n1_port= $NODE_MYPORT_1
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

--echo Performing State Transfer on a server that has been temporarily disconnected

--connection $n1
CREATE TABLE t1 (f1 CHAR(255)) ENGINE=InnoDB;
SET AUTOCOMMIT=OFF;
START TRANSACTION;
INSERT INTO t1 VALUES ('node1_committed_before');
INSERT INTO t1 VALUES ('node1_committed_before');
INSERT INTO t1 VALUES ('node1_committed_before');
INSERT INTO t1 VALUES ('node1_committed_before');
INSERT INTO t1 VALUES ('node1_committed_before');
COMMIT;

--connection $n2
SET AUTOCOMMIT=OFF;
START TRANSACTION;
INSERT INTO t1 VALUES ('node2_committed_before');
INSERT INTO t1 VALUES ('node2_committed_before');
INSERT INTO t1 VALUES ('node2_committed_before');
INSERT INTO t1 VALUES ('node2_committed_before');
INSERT INTO t1 VALUES ('node2_committed_before');
COMMIT;

--source suite/galera/include/galera_unload_provider.inc

--connection $n1
--let $wait_condition = SELECT VARIABLE_VALUE = 1 FROM INFORMATION_SCHEMA.GLOBAL_STATUS WHERE VARIABLE_NAME = 'wsrep_cluster_size'
--source include/wait_condition.inc

SET AUTOCOMMIT=OFF;
START TRANSACTION;
INSERT INTO t1 VALUES ('node1_committed_during');
INSERT INTO t1 VALUES ('node1_committed_during');
INSERT INTO t1 VALUES ('node1_committed_during');
INSERT INTO t1 VALUES ('node1_committed_during');
INSERT INTO t1 VALUES ('node1_committed_during');
COMMIT;

START TRANSACTION;
INSERT INTO t1 VALUES ('node1_to_be_committed_after');
INSERT INTO t1 VALUES ('node1_to_be_committed_after');
INSERT INTO t1 VALUES ('node1_to_be_committed_after');
INSERT INTO t1 VALUES ('node1_to_be_committed_after');
INSERT INTO t1 VALUES ('node1_to_be_committed_after');

--connect node_1a_galera_st_disconnect_slave, 127.0.0.1, root, , test, $n1_port
SET AUTOCOMMIT=OFF;
START TRANSACTION;
INSERT INTO t1 VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 VALUES ('node1_to_be_rollbacked_after');

--connection $n2
--source suite/galera/include/galera_load_provider.inc

--let $wait_condition = SELECT VARIABLE_VALUE = 2 FROM INFORMATION_SCHEMA.GLOBAL_STATUS WHERE VARIABLE_NAME = 'wsrep_cluster_size'
--source include/wait_condition.inc

SET AUTOCOMMIT=OFF;
START TRANSACTION;
INSERT INTO t1 VALUES ('node2_committed_after');
INSERT INTO t1 VALUES ('node2_committed_after');
INSERT INTO t1 VALUES ('node2_committed_after');
INSERT INTO t1 VALUES ('node2_committed_after');
INSERT INTO t1 VALUES ('node2_committed_after');
COMMIT;

--connection $n1
INSERT INTO t1 VALUES ('node1_to_be_committed_after');
INSERT INTO t1 VALUES ('node1_to_be_committed_after');
INSERT INTO t1 VALUES ('node1_to_be_committed_after');
INSERT INTO t1 VALUES ('node1_to_be_committed_after');
INSERT INTO t1 VALUES ('node1_to_be_committed_after');
COMMIT;

SET AUTOCOMMIT=OFF;
START TRANSACTION;
INSERT INTO t1 VALUES ('node1_committed_after');
INSERT INTO t1 VALUES ('node1_committed_after');
INSERT INTO t1 VALUES ('node1_committed_after');
INSERT INTO t1 VALUES ('node1_committed_after');
INSERT INTO t1 VALUES ('node1_committed_after');
COMMIT;

--connection node_1a_galera_st_disconnect_slave
INSERT INTO t1 VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 VALUES ('node1_to_be_rollbacked_after');
INSERT INTO t1 VALUES ('node1_to_be_rollbacked_after');
ROLLBACK;

SELECT COUNT(*) = 35 FROM t1;
SELECT COUNT(*) = 0 FROM (SELECT COUNT(*) AS c, f1 FROM t1 GROUP BY f1 HAVING c NOT IN (5, 10)) AS a1;
COMMIT;
SET AUTOCOMMIT=ON;

--connection $n1
SELECT COUNT(*) = 35 FROM t1;
SELECT COUNT(*) = 0 FROM (SELECT COUNT(*) AS c, f1 FROM t1 GROUP BY f1 HAVING c NOT IN (5, 10)) AS a1;
DROP TABLE t1;
COMMIT;
SET AUTOCOMMIT=ON;
