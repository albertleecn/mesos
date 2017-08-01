// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <process/id.hpp>

#include <stout/foreach.hpp>
#include <stout/hashmap.hpp>

#include "linux/cgroups.hpp"

#include "slave/containerizer/mesos/isolators/cgroups/subsystems/blkio.hpp"

namespace cfq = cgroups::blkio::cfq;
namespace throttle = cgroups::blkio::throttle;

using process::Failure;
using process::Future;
using process::Owned;

using std::string;
using std::vector;

using cgroups::blkio::Device;
using cgroups::blkio::Operation;

namespace mesos {
namespace internal {
namespace slave {

Try<Owned<Subsystem>> BlkioSubsystem::create(
    const Flags& flags,
    const string& hierarchy)
{
  return Owned<Subsystem>(new BlkioSubsystem(flags, hierarchy));
}


BlkioSubsystem::BlkioSubsystem(
    const Flags& _flags,
    const string& _hierarchy)
  : ProcessBase(process::ID::generate("cgroups-blkio-subsystem")),
    Subsystem(_flags, _hierarchy) {}


static void setValue(
    const cgroups::blkio::Value& statValue,
    CgroupInfo::Blkio::Value* value)
{
  if (statValue.op.isNone()) {
    value->set_op(CgroupInfo::Blkio::UNKNOWN);
  } else {
    switch(statValue.op.get()) {
      case Operation::TOTAL:
        value->set_op(CgroupInfo::Blkio::TOTAL);
        break;
      case Operation::READ:
        value->set_op(CgroupInfo::Blkio::READ);
        break;
      case Operation::WRITE:
        value->set_op(CgroupInfo::Blkio::WRITE);
        break;
      case Operation::SYNC:
        value->set_op(CgroupInfo::Blkio::SYNC);
        break;
      case Operation::ASYNC:
        value->set_op(CgroupInfo::Blkio::ASYNC);
        break;
    }
  }

  value->set_value(statValue.value);
}


Future<ResourceStatistics> BlkioSubsystem::usage(
    const ContainerID& containerId,
    const string& cgroup)
{
  hashmap<dev_t, CgroupInfo::Blkio::CFQ::Statistics> cfq;
  hashmap<dev_t, CgroupInfo::Blkio::CFQ::Statistics> cfqRecursive;
  hashmap<dev_t, CgroupInfo::Blkio::Throttling::Statistics> throttling;

  CgroupInfo::Blkio::CFQ::Statistics totalCfq;
  CgroupInfo::Blkio::CFQ::Statistics totalCfqRecursive;
  CgroupInfo::Blkio::Throttling::Statistics totalThrottling;

  // Get CFQ statistics.
  Try<vector<cgroups::blkio::Value>> time = cfq::time(hierarchy, cgroup);
  if (time.isError()) {
    return Failure(time.error());
  }

  foreach (const cgroups::blkio::Value& value, time.get()) {
    if (value.device.isNone()) {
      totalCfq.set_time(value.value);
    } else {
      cfq[value.device.get()].set_time(value.value);
    }
  }

  Try<vector<cgroups::blkio::Value>> sectors = cfq::sectors(hierarchy, cgroup);
  if (sectors.isError()) {
    return Failure(sectors.error());
  }

  foreach (const cgroups::blkio::Value& value, sectors.get()) {
    if (value.device.isNone()) {
      totalCfq.set_sectors(value.value);
    } else {
      cfq[value.device.get()].set_sectors(value.value);
    }
  }

  Try<vector<cgroups::blkio::Value>> io_service_bytes =
    cfq::io_service_bytes(hierarchy, cgroup);

  if (io_service_bytes.isError()) {
    return Failure(io_service_bytes.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_service_bytes.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? cfq[statValue.device.get()].add_io_service_bytes()
      : totalCfq.add_io_service_bytes();

    setValue(statValue, value);
  }

  Try<vector<cgroups::blkio::Value>> io_serviced =
    cfq::io_serviced(hierarchy, cgroup);

  if (io_serviced.isError()) {
    return Failure(io_serviced.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_serviced.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? cfq[statValue.device.get()].add_io_serviced()
      : totalCfq.add_io_serviced();

    setValue(statValue, value);
  }

  Try<vector<cgroups::blkio::Value>> io_service_time =
    cfq::io_service_time(hierarchy, cgroup);

  if (io_service_time.isError()) {
    return Failure(io_service_time.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_service_time.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? cfq[statValue.device.get()].add_io_service_time()
      : totalCfq.add_io_service_time();

    setValue(statValue, value);
  }

  Try<vector<cgroups::blkio::Value>> io_wait_time =
    cfq::io_wait_time(hierarchy, cgroup);

  if (io_wait_time.isError()) {
    return Failure(io_wait_time.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_wait_time.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? cfq[statValue.device.get()].add_io_wait_time()
      : totalCfq.add_io_wait_time();

    setValue(statValue, value);
  }

  Try<vector<cgroups::blkio::Value>> io_merged =
    cfq::io_merged(hierarchy, cgroup);

  if (io_merged.isError()) {
    return Failure(io_merged.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_merged.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? cfq[statValue.device.get()].add_io_merged()
      : totalCfq.add_io_merged();

    setValue(statValue, value);
  }

  Try<vector<cgroups::blkio::Value>> io_queued =
    cfq::io_queued(hierarchy, cgroup);

  if (io_queued.isError()) {
    return Failure(io_queued.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_queued.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? cfq[statValue.device.get()].add_io_queued()
      : totalCfq.add_io_queued();

    setValue(statValue, value);
  }

  // Get CFQ recursive statistics (blkio.*_recursive).
  time = cfq::time_recursive(hierarchy, cgroup);
  if (time.isError()) {
    return Failure(time.error());
  }

  foreach (const cgroups::blkio::Value& value, time.get()) {
    if (value.device.isNone()) {
      totalCfqRecursive.set_time(value.value);
    } else {
      cfqRecursive[value.device.get()].set_time(value.value);
    }
  }

  sectors = cfq::sectors_recursive(hierarchy, cgroup);
  if (sectors.isError()) {
    return Failure(sectors.error());
  }

  foreach (const cgroups::blkio::Value& value, sectors.get()) {
    if (value.device.isNone()) {
      totalCfqRecursive.set_sectors(value.value);
    } else {
      cfqRecursive[value.device.get()].set_sectors(value.value);
    }
  }

  io_service_bytes = cfq::io_service_bytes_recursive(hierarchy, cgroup);
  if (io_service_bytes.isError()) {
    return Failure(io_service_bytes.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_service_bytes.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? cfqRecursive[statValue.device.get()].add_io_service_bytes()
      : totalCfqRecursive.add_io_service_bytes();

    setValue(statValue, value);
  }

  io_serviced = cfq::io_serviced_recursive(hierarchy, cgroup);
  if (io_serviced.isError()) {
    return Failure(io_serviced.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_serviced.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? cfqRecursive[statValue.device.get()].add_io_serviced()
      : totalCfqRecursive.add_io_serviced();

    setValue(statValue, value);
  }

  io_service_time = cfq::io_service_time_recursive(hierarchy, cgroup);
  if (io_service_time.isError()) {
    return Failure(io_service_time.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_service_time.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? cfqRecursive[statValue.device.get()].add_io_service_time()
      : totalCfqRecursive.add_io_service_time();

    setValue(statValue, value);
  }

  io_wait_time = cfq::io_wait_time_recursive(hierarchy, cgroup);
  if (io_wait_time.isError()) {
    return Failure(io_wait_time.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_wait_time.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? cfqRecursive[statValue.device.get()].add_io_wait_time()
      : totalCfqRecursive.add_io_wait_time();

    setValue(statValue, value);
  }

  io_merged = cfq::io_merged_recursive(hierarchy, cgroup);
  if (io_merged.isError()) {
    return Failure(io_merged.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_merged.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? cfqRecursive[statValue.device.get()].add_io_merged()
      : totalCfqRecursive.add_io_merged();

    setValue(statValue, value);
  }

  io_queued = cfq::io_queued_recursive(hierarchy, cgroup);
  if (io_queued.isError()) {
    return Failure(io_queued.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_queued.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? cfqRecursive[statValue.device.get()].add_io_queued()
      : totalCfqRecursive.add_io_queued();

    setValue(statValue, value);
  }

  // Get throttling statistics.
  io_serviced = throttle::io_serviced(hierarchy, cgroup);
  if (io_serviced.isError()) {
    return Failure(io_serviced.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_serviced.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? throttling[statValue.device.get()].add_io_serviced()
      : totalThrottling.add_io_serviced();

    setValue(statValue, value);
  }

  io_service_bytes = throttle::io_service_bytes(hierarchy, cgroup);
  if (io_service_bytes.isError()) {
    return Failure(io_service_bytes.error());
  }

  foreach (const cgroups::blkio::Value& statValue, io_service_bytes.get()) {
    CgroupInfo::Blkio::Value* value = statValue.device.isSome()
      ? throttling[statValue.device.get()].add_io_service_bytes()
      : totalThrottling.add_io_service_bytes();

    setValue(statValue, value);
  }

  // Add up resource statistics.
  ResourceStatistics result;
  CgroupInfo::Blkio::Statistics* stat = result.mutable_blkio_statistics();

  foreachkey (dev_t dev, cfq) {
    cfq[dev].mutable_device()->set_major(major(dev));
    cfq[dev].mutable_device()->set_minor(minor(dev));
    stat->add_cfq()->CopyFrom(cfq[dev]);
  }

  foreachkey (dev_t dev, cfqRecursive) {
    cfqRecursive[dev].mutable_device()->set_major(major(dev));
    cfqRecursive[dev].mutable_device()->set_minor(minor(dev));
    stat->add_cfq_recursive()->CopyFrom(cfqRecursive[dev]);
  }

  foreachkey (dev_t dev, throttling) {
    throttling[dev].mutable_device()->set_major(major(dev));
    throttling[dev].mutable_device()->set_minor(minor(dev));
    stat->add_throttling()->CopyFrom(throttling[dev]);
  }

  stat->add_cfq()->CopyFrom(totalCfq);
  stat->add_cfq_recursive()->CopyFrom(totalCfqRecursive);
  stat->add_throttling()->CopyFrom(totalThrottling);

  return result;
}

} // namespace slave {
} // namespace internal {
} // namespace mesos {
