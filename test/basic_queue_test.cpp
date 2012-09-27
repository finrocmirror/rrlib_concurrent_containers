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
/*!\file    rrlib/concurrent_containers/test/basic_queue_test.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-09-26
 *
 * This tests whether basic operations on queues work correctly.
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/logging/messages.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/concurrent_containers/tQueue.h"

//----------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------
#include <cassert>

//----------------------------------------------------------------------
// Namespace usage
//----------------------------------------------------------------------
using namespace rrlib::concurrent_containers;

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------
struct tTestType : public tQueueable, public tQueueableSingleThreaded
{
  int value;

  tTestType(int value) : value(value) {}
};

template <typename Q>
std::unique_ptr<tTestType> DequeueElement(Q& queue)
{
  bool success = false;
  std::unique_ptr<tTestType> element = queue.Dequeue(success);
  assert(success == (element.get() != NULL) && "Setting success seems broken");
  if (element)
  {
    RRLIB_LOG_PRINT(USER, "  Dequeued ", element->value);
  }
  else
  {
    RRLIB_LOG_PRINT(USER, "  Dequeued nothing");
  }
  return element;
}

template <tQueueConcurrency QC>
void TestQueue()
{
  RRLIB_LOG_PRINT(USER, "Testing tQueue<std::unique_ptr<tTestType>, tQueueConcurrency::", make_builder::GetEnumString(QC), ">");
  tQueue<std::unique_ptr<tTestType>, QC> q;

  RRLIB_LOG_PRINT(USER, " Enqueueing ten elements: 1 to 10");
  for (int i = 1; i <= 10; i++)
  {
    q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i)));
  }
  RRLIB_LOG_PRINT(USER, " Dequeueing twelve elements:");
  for (int i = 0; i < 12; i++)
  {
    DequeueElement(q);
  }

  RRLIB_LOG_PRINT(USER, " Enqueueing ten elements: 11 to 20");
  for (int i = 11; i <= 20; i++)
  {
    q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i)));
  }
  RRLIB_LOG_PRINT(USER, " Dequeueing five elements and enqueueing them again:");
  for (int i = 0; i < 5; i++)
  {
    q.Enqueue(DequeueElement(q));
  }
  RRLIB_LOG_PRINT(USER, " Dequeueing twelve elements:");
  for (int i = 0; i < 12; i++)
  {
    DequeueElement(q);
  }

  RRLIB_LOG_PRINT(USER, " Performing one enqueue and dequeue operation 5 times (elements 100 to 104):");
  for (int i = 0; i < 5; i++)
  {
    q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i + 100)));
    DequeueElement(q);
  }

  RRLIB_LOG_PRINT(USER, " ");
}

int main(int, char**)
{
  TestQueue<tQueueConcurrency::NONE>();
  TestQueue<tQueueConcurrency::SINGLE_READER_AND_WRITER_FAST>();
  TestQueue<tQueueConcurrency::MULTIPLE_WRITERS>();
  TestQueue<tQueueConcurrency::MULTIPLE_WRITERS_FAST>();
  TestQueue<tQueueConcurrency::MULTIPLE_READERS_FAST>();
  TestQueue<tQueueConcurrency::FULL_FAST>();
  return 0;
}
