/*
  Copyright (c) DataStax, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "integration.hpp"

/**
 * Prepared integration tests; common operations
 */
class PreparedTests : public Integration {
  void SetUp() {
    is_keyspace_change_requested_ = false;
    Integration::SetUp();
  }
};

/**
 * Execute a statement that forces a re-prepare resulting in a new prepared ID that fails fast and
 * returns an error.
 *
 * This test will create a new table, prepare a statement using a fully qualified query, update the
 * default keyspace, then drop and re-create the table to force the server to invalidate the
 * prepared ID. After the table is dropped the prepared statement will be used to execute an insert
 * query that will result in an error being returned when re-using the original prepared statement.
 *
 * @see: https://issues.apache.org/jira/browse/CASSANDRA-15252 (Server version restriction may need
 *       to be added if/when Apache Cassandra issue is addressed.
 *
 * @test_category error
 * @test_category queries:prepared
 * @since core:2.14.0
 * @expected_result Re-prepare will fail fast and return error.
 */
CASSANDRA_INTEGRATION_TEST_F(PreparedTests, FailFastWhenPreparedIDChangesDuringReprepare) {
  CHECK_FAILURE;

  // Create the table and initial prepared statement
  session_.execute(format_string(CASSANDRA_KEY_VALUE_QUALIFIED_TABLE_FORMAT, keyspace_name_.c_str(),
                                 table_name_.c_str(), "int", "int"));
  Prepared insert_prepared =
      session_.prepare(format_string(CASSANDRA_KEY_VALUE_QUALIFIED_INSERT_FORMAT,
                                     keyspace_name_.c_str(), table_name_.c_str(), "?", "?"));

  // Update the current keyspace for the session
  ASSERT_TRUE(use_keyspace(keyspace_name_));

  // Drop and re-create the table to invalidate the prepared statement on the server
  drop_table(table_name_);
  session_.execute(format_string(CASSANDRA_KEY_VALUE_QUALIFIED_TABLE_FORMAT, keyspace_name_.c_str(),
                                 table_name_.c_str(), "int", "int"));

  // Execute the insert statement and validate the error code
  logger_.add_critera("ID mismatch while trying to prepare query");
  Statement insert_statement = insert_prepared.bind();
  insert_statement.bind<Integer>(0, Integer(0));
  insert_statement.bind<Integer>(1, Integer(1));
  Result result = session_.execute(insert_statement, false);
  EXPECT_TRUE(contains(result.error_message(), "ID mismatch while trying to prepare query"));
}

/**
 * Execute a statement that forces a re-prepare resulting in a same prepared ID.
 *
 * This test will connect to a cluster and use a keyspace, prepare a statement using a unqualified
 * query, then drop and re-create the table to force the server to invalidate the
 * prepared ID. After the table is dropped the prepared statement will be used to execute an insert
 * query that will result the statement being re-prepared and the insert statement succeeding.
 *
 * @test_category queries:prepared
 * @since core:1.0.0
 * @expected_result Re-prepare will correctly execute the insert statement.
 */
CASSANDRA_INTEGRATION_TEST_F(PreparedTests, PreparedIDUnchangedDuringReprepare) {
  CHECK_FAILURE;

  // Allow for unqualified queries
  use_keyspace(keyspace_name_);

  // Create the table and initial prepared statement
  session_.execute(
      format_string(CASSANDRA_KEY_VALUE_TABLE_FORMAT, table_name_.c_str(), "int", "int"));
  Prepared insert_prepared = session_.prepare(
      format_string(CASSANDRA_KEY_VALUE_INSERT_FORMAT, table_name_.c_str(), "?", "?"));

  // Drop and re-create the table to invalidate the prepared statement on the server
  drop_table(table_name_);
  session_.execute(
      format_string(CASSANDRA_KEY_VALUE_TABLE_FORMAT, table_name_.c_str(), "int", "int"));

  // Execute the insert statement and validate success
  logger_.add_critera("Prepared query with ID");
  Statement insert_statement = insert_prepared.bind();
  insert_statement.bind<Integer>(0, Integer(0));
  insert_statement.bind<Integer>(1, Integer(1));
  Result result = session_.execute(insert_statement, false);
  EXPECT_EQ(CASS_OK, result.error_code());
  EXPECT_EQ(1u, logger_.count());
}
