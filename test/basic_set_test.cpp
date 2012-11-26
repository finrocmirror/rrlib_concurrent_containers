//
// You received this file as part of RRLib
// Robotics Research Library
//
// Copyright (C) Finroc GbR (finroc.org)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//----------------------------------------------------------------------
/*!\file    rrlib/concurrent_containers/test/basic_set_test.cpp
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
using namespace rrlib::concurrent_containers;

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
void TestSet(TSet& set)
{
  RRLIB_LOG_PRINT(USER, " Iterating over elements:");
  for (auto it = set.Begin(); it != set.End(); ++it)
  {
    RRLIB_LOG_PRINT(USER, "  ", *it);
  }

  RRLIB_LOG_PRINT(USER, " Adding twenty elements: 1 to 20");
  for (int i = 1; i <= 20; ++i)
  {
    set.Add(i);
  }

  RRLIB_LOG_PRINT(USER, " Iterating over elements:");
  for (auto it = set.Begin(); it != set.End(); ++it)
  {
    RRLIB_LOG_PRINT(USER, "  ", *it);
  }

  // Make sure that const iterator compiles
  const TSet& const_set = set;
  for (auto it = const_set.Begin(); it != const_set.End(); ++it);

  RRLIB_LOG_PRINT(USER, " Removing every second element.");
  for (auto it = set.Begin(); it != set.End(); ++it)
  {
    set.Remove(it);
    ++it;
  }
  RRLIB_LOG_PRINT(USER, " Removing twenty.");
  set.Remove(20);

  RRLIB_LOG_PRINT(USER, " Adding elements 1 to 4.");
  for (int i = 1; i <= 4; ++i)
  {
    set.Add(i);
  }

  RRLIB_LOG_PRINT(USER, " Iterating over elements now:");
  for (auto it = set.Begin(); it != set.End(); ++it)
  {
    RRLIB_LOG_PRINT(USER, "  ", *it);
  }
}

int main(int, char**)
{
  {
    RRLIB_LOG_PRINT(USER, "Testing tSet<int, tAllowDuplicates::NO, rrlib::thread::tMutex, set::storage::ArrayChunkBased<2, 6>>");
    tSet<int, tAllowDuplicates::NO, rrlib::thread::tMutex, set::storage::ArrayChunkBased<2, 6>> set;
    TestSet(set);
  }

  {
    RRLIB_LOG_PRINT(USER, "Testing tSet<int, tAllowDuplicates::NO, rrlib::thread::tMutex, set::storage::ArrayChunkBased<2, 6, true>>");
    tSet<int, tAllowDuplicates::NO, rrlib::thread::tMutex, set::storage::ArrayChunkBased<2, 6, true>> set;
    TestSet(set);
  }

  {
    RRLIB_LOG_PRINT(USER, "Testing tSet<int, tAllowDuplicates::YES, rrlib::thread::tNoMutex, set::storage::ArrayChunkBased<4, 8>>");
    tSet<int, tAllowDuplicates::YES, rrlib::thread::tNoMutex, set::storage::ArrayChunkBased<4, 8>> set;
    TestSet(set);
  }

  return 0;
}

