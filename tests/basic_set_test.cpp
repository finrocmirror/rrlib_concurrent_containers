//
// You received this file as part of RRLib
// Robotics Research Library
//
// Copyright (C) Finroc GbR (finroc.org)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//----------------------------------------------------------------------
/*!\file    rrlib/concurrent_containers/tests/basic_set_test.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-08
 *
 * Tests basic functionality of sets with different combinations of template parameters.
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/logging/messages.h"
#include "rrlib/util/tUnitTestSuite.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/concurrent_containers/tSet.h"

//----------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------
#include <cassert>

//----------------------------------------------------------------------
// Namespace usage
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Namespace declaration
//----------------------------------------------------------------------
namespace rrlib
{
namespace concurrent_containers
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

/*!
 * Test int-sets
 */
template <typename TSet>
void TestSet(TSet& set, bool duplicates_allowed)
{
  RRLIB_LOG_PRINT(DEBUG_VERBOSE_1, " Iterating over elements:");
  for (auto it = set.Begin(); it != set.End(); ++it)
  {
    RRLIB_LOG_PRINT(DEBUG_VERBOSE_1, "  ", *it);
  }

  RRLIB_LOG_PRINT(DEBUG_VERBOSE_1, " Adding twenty elements: 1 to 20");
  for (int i = 1; i <= 20; ++i)
  {
    set.Add(i);
  }

  RRLIB_LOG_PRINT(DEBUG_VERBOSE_1, " Iterating over elements:");
  int i = 0;
  for (auto it = set.Begin(); it != set.End(); ++it)
  {
    RRLIB_LOG_PRINT(DEBUG_VERBOSE_1, "  ", *it);
    i++;
    RRLIB_UNIT_TESTS_EQUALITY(*it, i);
  }

  // Make sure that const iterator compiles
  const TSet& const_set = set;
  for (auto it = const_set.Begin(); it != const_set.End(); ++it);

  RRLIB_LOG_PRINT(DEBUG_VERBOSE_1, " Removing every second element.");
  for (auto it = set.Begin(); it != set.End(); ++it)
  {
    set.Remove(it);
    ++it;
  }
  RRLIB_LOG_PRINT(DEBUG_VERBOSE_1, " Removing twenty.");
  set.Remove(20);

  RRLIB_LOG_PRINT(DEBUG_VERBOSE_1, " Adding elements 1 to 4.");
  for (int i = 1; i <= 4; ++i)
  {
    set.Add(i);
  }

  RRLIB_LOG_PRINT(DEBUG_VERBOSE_1, " Iterating over elements now:");
  i = 0;
  for (auto it = set.Begin(); it != set.End(); ++it)
  {
    RRLIB_LOG_PRINT(DEBUG_VERBOSE_1, "  ", *it);
    RRLIB_UNIT_TESTS_ASSERT(((*it % 2) == 0 || *it <= 4) && (*it <= 18));
    i += (i >= 4) ? 2 : 1;
    RRLIB_UNIT_TESTS_ASSERT(duplicates_allowed || (*it == i));
  }
}

class BasicSetTest : public util::tUnitTestSuite
{
  RRLIB_UNIT_TESTS_BEGIN_SUITE(BasicSetTest);
  RRLIB_UNIT_TESTS_ADD_TEST(Test);
  RRLIB_UNIT_TESTS_END_SUITE;

  void Test()
  {
    {
      RRLIB_LOG_PRINT(DEBUG_VERBOSE_1, "Testing tSet<int, tAllowDuplicates::NO, rrlib::thread::tMutex, set::storage::ArrayChunkBased<2, 6>>");
      tSet<int, tAllowDuplicates::NO, rrlib::thread::tMutex, set::storage::ArrayChunkBased<2, 6>> set;
      TestSet(set, false);
    }

    {
      RRLIB_LOG_PRINT(DEBUG_VERBOSE_1, "Testing tSet<int, tAllowDuplicates::NO, rrlib::thread::tMutex, set::storage::ArrayChunkBased<2, 6, true>>");
      tSet<int, tAllowDuplicates::NO, rrlib::thread::tMutex, set::storage::ArrayChunkBased<2, 6, true>> set;
      TestSet(set, false);
    }

    {
      RRLIB_LOG_PRINT(DEBUG_VERBOSE_1, "Testing tSet<int, tAllowDuplicates::YES, rrlib::thread::tNoMutex, set::storage::ArrayChunkBased<4, 8>>");
      tSet<int, tAllowDuplicates::YES, rrlib::thread::tNoMutex, set::storage::ArrayChunkBased<4, 8>> set;
      TestSet(set, true);
    }
  }

};

RRLIB_UNIT_TESTS_REGISTER_SUITE(BasicSetTest);

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
