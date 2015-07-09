#include <gtest/gtest.h>

#include <gmock/gmock.h>

#include <process/clock.hpp>
#include <process/time.hpp>

#include <stout/duration.hpp>
#include <stout/gtest.hpp>
#include <stout/os.hpp>

using namespace process;


TEST(TimeTest, Arithmetic)
{
  Time t = Time::epoch() + Weeks(1000);
  t -= Weeks(1);
  EXPECT_EQ(Time::epoch() + Weeks(999), t);

  t += Weeks(2);
  EXPECT_EQ(Time::epoch() + Weeks(1001), t);

  EXPECT_EQ(t, Time::epoch() + Weeks(1000) + Weeks(1));
  EXPECT_EQ(t, Time::epoch() + Weeks(1002) - Weeks(1));

  EXPECT_EQ(Weeks(1),
            (Time::epoch() + Weeks(1000)) - (Time::epoch() + Weeks(999)));
}


TEST(TimeTest, Now)
{
  Time t1 = Clock::now();
  os::sleep(Microseconds(10));
  ASSERT_LT(Microseconds(10), Clock::now() - t1);
}


// Tests stream manipulator which formats Time object into RFC 1123
// (HTTP Date) format.
TEST(TimeTest, RFC1123Output)
{
  EXPECT_EQ(
      "Thu, 01 Jan 1970 00:00:00 GMT",
      stringify(RFC1123(Time::epoch())));

  EXPECT_EQ(
      "Thu, 02 Mar 1989 00:00:00 GMT",
      stringify(RFC1123(Time::epoch() + Weeks(1000))));

  EXPECT_EQ(
      "Thu, 02 Mar 1989 00:00:00 GMT",
      stringify(RFC1123(Time::epoch() + Weeks(1000) + Nanoseconds(1))));

  EXPECT_EQ(
      "Thu, 02 Mar 1989 00:00:01 GMT",
      stringify(RFC1123(Time::epoch() + Weeks(1000) + Seconds(1))));

  EXPECT_EQ(
      "Fri, 11 Apr 2262 23:47:16 GMT",
      stringify(RFC1123(Time::max())));
}


// Tests stream manipulator which formats Time object into RFC 3339
// format.
TEST(TimeTest, RFC3339Output)
{
  EXPECT_EQ(
      "1970-01-01 00:00:00+00:00",
      stringify(RFC3339(Time::epoch())));

  EXPECT_EQ(
      "1989-03-02 00:00:00+00:00",
      stringify(RFC3339(Time::epoch() + Weeks(1000))));

  EXPECT_EQ(
      "1989-03-02 00:00:00.000000001+00:00",
      stringify(RFC3339(Time::epoch() + Weeks(1000) + Nanoseconds(1))));

  EXPECT_EQ(
      "1989-03-02 00:00:00.000001000+00:00",
      stringify(RFC3339(Time::epoch() + Weeks(1000) + Microseconds(1))));

  EXPECT_EQ(
      "2262-04-11 23:47:16.854775807+00:00",
      stringify(RFC3339(Time::max())));
}


TEST(TimeTest, Output)
{
  EXPECT_EQ("1989-03-02 00:00:00+00:00",
            stringify(Time::epoch() + Weeks(1000)));
  EXPECT_EQ("1989-03-02 00:00:00.000000001+00:00",
            stringify(Time::epoch() + Weeks(1000) + Nanoseconds(1)));
  EXPECT_EQ("1989-03-02 00:00:00.000001000+00:00",
            stringify(Time::epoch() + Weeks(1000) + Microseconds(1)));
}
