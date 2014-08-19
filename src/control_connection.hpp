/*
  Copyright 2014 DataStax

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

#include "connection.hpp"
#include "load_balancing.hpp"
#include "macros.hpp"
#include "scoped_ptr.hpp"

#ifndef __CASS_CONTROL_CONNECTION_HPP_INCLUDED__
#define __CASS_CONTROL_CONNECTION_HPP_INCLUDED__

namespace cass {

class Session;
class Timer;

class ControlConnection {
public:
  typedef boost::function1<void, ControlConnection*> Callback;
  typedef boost::function2<void, CassError, const std::string&> ErrorCallback;

  enum ControlState {
    CONTROL_STATE_NEW,
    CONTROL_STATE_CONNECTED,
    CONTROL_STATE_CLOSED
  };

  ControlConnection()
    : session_(NULL)
    , state_(CONTROL_STATE_NEW)
    , connection_(NULL)
    , reconnect_timer_(NULL) {}

  void set_session(Session* session) {
    session_ = session;
  }

  void set_ready_callback(Callback callback) {
    ready_callback_ = callback;
  }

  void set_error_callback(ErrorCallback callback) {
    error_callback_ = callback;
  }

  void connect();
  void close();

private:
  void schedule_reconnect(uint64_t ms = 0);
  void reconnect();

  void on_connection_ready(Connection* connection);
  void on_connection_closed(Connection* connection);
  void on_reconnect(Timer* timer);

private:
  Session* session_;
  ControlState state_;
  Connection* connection_;
  ScopedPtr<QueryPlan> query_plan_;
  Callback ready_callback_;
  ErrorCallback error_callback_;
  Timer* reconnect_timer_;

private:
  DISALLOW_COPY_AND_ASSIGN(ControlConnection);
};

} // namespace cass

#endif
