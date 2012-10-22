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
/*!\file    rrlib/concurrent_containers/test/atomic_int64_stress_test.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-22
 *
 * Performs stress test on std::atomic<uint64_t> to check
 * whether torn writes occur.
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <atomic>
#include <thread>
#include <vector>
#include <cstdint>
//#include <tbb/atomic.h>

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------
#include <cassert>

//----------------------------------------------------------------------
// Namespace usage
//----------------------------------------------------------------------
using namespace std;

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------
std::atomic<uint64_t> tested(0);

void Test(uint64_t thread_no)
{
  uint64_t id = (thread_no << 32) | thread_no; // low and high int32 are identical
  for (uint32_t i = 0; i < 0xFFFFFF; i++)
  {
    uint64_t current_value = tested.load();  // produces torn reads on 32-bit Ubuntu 12.04
    //uint64_t current_value = tested.fetch_add(0);  // this works perfectly
    do
    {
      assert((current_value >> 32) == (current_value & 0xFFFFFFFF) && "detected torn write"); // check that low and high int32 are identical
    }
    while (!tested.compare_exchange_strong(current_value, id));
  }
}

/*tbb::atomic<uint64_t> tested_tbb;

void TestTbb(uint64_t thread_no)
{
  uint64_t id = (thread_no << 32) | thread_no; // low and high int32 are identical
  for (uint i = 0; i < 0xFFFFFF; i++)
  {
    uint64_t current_value = tested_tbb.load<tbb::full_fence>();
    bool again = false;
    do
    {
      assert((current_value >> 32) == (current_value & 0xFFFFFFFF) && "detected torn write"); // check that low and high int32 are identical
      uint64_t v = tested_tbb.compare_and_swap(id, current_value);
      again = (v != current_value);
    } while (again);
  }
}*/

int main(int, char**)
{
  //tested_tbb = 0;
  std::vector<std::thread> threads;
  for (int i = 0; i < 3; i++)
  {
    threads.emplace_back(&Test, i);
  }
  for (auto it = threads.begin(); it != threads.end(); ++it)
  {
    it->join();
  }

  return 0;
}

