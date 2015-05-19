/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <process/delay.hpp>
#include <process/dispatch.hpp>
#include <process/process.hpp>

#include <stout/error.hpp>

#include "slave/resource_estimator.hpp"

using namespace process;

using std::string;

namespace mesos {
namespace slave {

Try<ResourceEstimator*> ResourceEstimator::create(const Option<string>& type)
{
  // TODO(jieyu): Support loading resource estimator from module.
  if (type.isNone()) {
    return new internal::slave::NoopResourceEstimator();
  }

  return Error("Unsupported resource estimator '" + type.get() + "'");
}

} // namespace slave {
} // namespace mesos {


namespace mesos {
namespace internal {
namespace slave {

class NoopResourceEstimatorProcess :
  public Process<NoopResourceEstimatorProcess>
{
public:
  NoopResourceEstimatorProcess(
      const lambda::function<void(const Resources&)>& _oversubscribe)
    : oversubscribe(_oversubscribe) {}

protected:
  virtual void initialize()
  {
    notify();
  }

  // Periodically notify the slave about oversubscribable resources.
  void notify()
  {
    oversubscribe(Resources());

    delay(Seconds(1), self(), &Self::notify);
  }

  const lambda::function<void(const Resources&)> oversubscribe;
};


NoopResourceEstimator::~NoopResourceEstimator()
{
  if (process.get() != NULL) {
    terminate(process.get());
    wait(process.get());
  }
}


Try<Nothing> NoopResourceEstimator::initialize(
    const lambda::function<void(const Resources&)>& oversubscribe)
{
  if (process.get() != NULL) {
    return Error("Noop resource estimator has already been initialized");
  }

  process.reset(new NoopResourceEstimatorProcess(oversubscribe));
  spawn(process.get());

  return Nothing();
}

} // namespace slave {
} // namespace internal {
} // namespace mesos {
