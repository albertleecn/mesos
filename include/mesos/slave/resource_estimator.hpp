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

#ifndef __MESOS_SLAVE_RESOURCE_ESTIMATOR_HPP__
#define __MESOS_SLAVE_RESOURCE_ESTIMATOR_HPP__

#include <string>

#include <mesos/resources.hpp>

#include <process/future.hpp>

#include <stout/lambda.hpp>
#include <stout/nothing.hpp>
#include <stout/option.hpp>
#include <stout/try.hpp>

namespace mesos {
namespace slave {

// A slave component used for oversubscription. It estimates and
// predicts the total resources used on the slave and informs the
// master about resources that can be oversubscribed.
class ResourceEstimator
{
public:
  // Create a resource estimator instance of the given type specified
  // by the user. If the type is not specified, a default resource
  // estimator instance will be created.
  static Try<ResourceEstimator*> create(const Option<std::string>& type);

  virtual ~ResourceEstimator() {}

  // Initializes this resource estimator. It registers a callback with
  // the resource estimator. The callback allows the resource
  // estimator to tell the slave about the current estimation of the
  // *maximum* amount of resources that can be oversubscribed on the
  // slave. A new estimation will invalidate all the previously
  // returned estimations. The slave will keep track of the most
  // recent estimation and periodically send it to the master.
  //
  // TODO(jieyu): Pass ResourceMonitor* once it's exposed.
  virtual Try<Nothing> initialize(
      const lambda::function<void(const Resources&)>& oversubscribe) = 0;
};

} // namespace slave {
} // namespace mesos {

#endif // __MESOS_SLAVE_RESOURCE_ESTIMATOR_HPP__
