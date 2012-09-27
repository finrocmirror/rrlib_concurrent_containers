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
/*!\file    rrlib/concurrent_containers/test/queue_stress_tests.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-09-27
 *
 * Performs stress tests for the available queue implementations -
 * involving multiple threads.
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/logging/messages.h"
#include <thread>
#include <cstring>

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
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------
const int cTHREADS = 3;
const int cBUFFERS = 100000000;

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------
class tTestType : public tQueueable
{
public:
  int element_no;
  int8_t thread_no;
  bool terminator;

  tTestType() : element_no(-1), thread_no(0), terminator(false) {}

  tTestType(bool terminator) : element_no(-1), thread_no(0), terminator(true) {}
};

template <tQueueConcurrency CONCURRENCY>
void EnqueueBuffers(tQueue<std::unique_ptr<tTestType>, CONCURRENCY>& queue, tTestType* buffers, size_t terminator_count)
{
  rrlib::time::tTimestamp start = rrlib::time::Now();
  for (int i = 0; i < cBUFFERS; i++, buffers++)
  {
    queue.Enqueue(std::unique_ptr<tTestType>(buffers));
  }

  RRLIB_LOG_PRINT(USER, "  Thread enqueued ", cBUFFERS, " elements in ", rrlib::time::ToString(rrlib::time::Now() - start), " starting at ", start, ".");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  for (size_t i = 0; i < terminator_count; i++, buffers++)
  {
    queue.Enqueue(std::unique_ptr<tTestType>(new tTestType(true)));
  }
}

std::atomic<int> dequeued_elements_global;

template <tQueueConcurrency CONCURRENCY>
void DequeueBuffers(tQueue<std::unique_ptr<tTestType>, CONCURRENCY>& queue, bool single_dequeue_thread, int terminator_count_to_stop)
{
  int next_element_index[cTHREADS];
  memset(next_element_index, 0, sizeof(next_element_index));
  int dequeued_elements = 0;
  rrlib::time::tTimestamp start = rrlib::time::Now();

  while (true)
  {
    std::unique_ptr<tTestType> buffer = queue.Dequeue();
    if (!buffer)
    {
      continue;
    }
    if (buffer->terminator)
    {
      terminator_count_to_stop--;
      if (terminator_count_to_stop <= 0)
      {
        dequeued_elements_global += dequeued_elements;
        RRLIB_LOG_PRINT(USER, "  Thread dequeued ", dequeued_elements, " elements in ", rrlib::time::ToString(rrlib::time::Now() - start), ".");
        return;
      }
      continue;
    }

    if (buffer->thread_no < 0 || buffer->thread_no >= cTHREADS)
    {
      RRLIB_LOG_PRINT(ERROR, "Invalid thread number: ", buffer->thread_no);
      abort();
    }

    if (single_dequeue_thread)
    {
      if (buffer->element_no != next_element_index[buffer->thread_no])
      {
        RRLIB_LOG_PRINT(ERROR, "Test failed: Element from thread ", buffer->thread_no, " - expected ", next_element_index[buffer->thread_no], " got ", buffer->element_no);
        abort();
      }
      next_element_index[buffer->thread_no]++;
    }
    else
    {
      if (buffer->element_no < next_element_index[buffer->thread_no])
      {
        RRLIB_LOG_PRINT(ERROR, "Test failed: Element from thread ", buffer->thread_no, " - expected >= ", next_element_index[buffer->thread_no], " got ", buffer->element_no);
        abort();
      }
      next_element_index[buffer->thread_no] = buffer->element_no + 1;
    }
    buffer.release();
    dequeued_elements++;
  }
}

int main(int, char**)
{
  RRLIB_LOG_PRINT(USER, "Allocating ", (cTHREADS * cBUFFERS * sizeof(tTestType)) / (1024 * 1024), " MB of buffers.");
  tTestType* buffers = new tTestType[cTHREADS * cBUFFERS];
  if (!buffers)
  {
    RRLIB_LOG_PRINT(ERROR, "Failed.");
    abort();
  }

  // Prepare buffers
  for (int i = 0; i < cTHREADS; i++)
  {
    for (int j = 0; j < cBUFFERS; j++)
    {
      buffers[i * cBUFFERS + j].element_no = j;
      buffers[i * cBUFFERS + j].thread_no = i;
    }
  }

  // Test SINGLE_READER_AND_WRITER_FAST
  {
    RRLIB_LOG_PRINT(USER, "Testing SINGLE_READER_AND_WRITER_FAST implementation:");
    dequeued_elements_global = 0;
    tQueue<std::unique_ptr<tTestType>, tQueueConcurrency::SINGLE_READER_AND_WRITER_FAST> queue;

    std::thread thread1(EnqueueBuffers<tQueueConcurrency::SINGLE_READER_AND_WRITER_FAST>, std::ref(queue), buffers, 2);
    DequeueBuffers<tQueueConcurrency::SINGLE_READER_AND_WRITER_FAST>(queue, true, 1);
    thread1.join();

    if (dequeued_elements_global != cBUFFERS)
    {
      RRLIB_LOG_PRINT(ERROR, dequeued_elements_global, " elements were dequeued in total. Expected ", cBUFFERS);
      abort();
    }
  }

  // Test MULTIPLE_WRITERS
  {
    RRLIB_LOG_PRINT(USER, "Testing MULTIPLE_WRITERS implementation with ", cTHREADS, " writer threads:");
    dequeued_elements_global = 0;
    tQueue<std::unique_ptr<tTestType>, tQueueConcurrency::MULTIPLE_WRITERS> queue;
    std::vector<std::thread> threads;

    for (int i = 0; i < cTHREADS; i++)
    {
      threads.emplace_back(EnqueueBuffers<tQueueConcurrency::MULTIPLE_WRITERS>, std::ref(queue), &(buffers[i * cBUFFERS]), 1);
    }
    DequeueBuffers<tQueueConcurrency::MULTIPLE_WRITERS>(queue, true, 3);
    for (int i = 0; i < cTHREADS; i++)
    {
      threads[i].join();
    }

    if (dequeued_elements_global != cTHREADS * cBUFFERS)
    {
      RRLIB_LOG_PRINT(ERROR, dequeued_elements_global, " elements were dequeued in total. Expected ", (cTHREADS * cBUFFERS));
      abort();
    }
  }

  // Test MULTIPLE_WRITERS_FAST
  {
    RRLIB_LOG_PRINT(USER, "Testing MULTIPLE_WRITERS_FAST implementation with ", cTHREADS, " writer threads:");
    dequeued_elements_global = 0;
    tQueue<std::unique_ptr<tTestType>, tQueueConcurrency::MULTIPLE_WRITERS_FAST> queue;
    std::vector<std::thread> threads;

    for (int i = 0; i < cTHREADS; i++)
    {
      threads.emplace_back(EnqueueBuffers<tQueueConcurrency::MULTIPLE_WRITERS_FAST>, std::ref(queue), &(buffers[i * cBUFFERS]), 2);
    }
    DequeueBuffers<tQueueConcurrency::MULTIPLE_WRITERS_FAST>(queue, true, 5);
    for (int i = 0; i < cTHREADS; i++)
    {
      threads[i].join();
    }

    if (dequeued_elements_global != cTHREADS * cBUFFERS)
    {
      RRLIB_LOG_PRINT(ERROR, dequeued_elements_global, " elements were dequeued in total. Expected ", (cTHREADS * cBUFFERS));
      abort();
    }
  }

  // Test MULTIPLE_READERS_FAST
  {
    RRLIB_LOG_PRINT(USER, "Testing MULTIPLE_READERS_FAST implementation with ", cTHREADS, " reader threads:");
    dequeued_elements_global = 0;
    tQueue<std::unique_ptr<tTestType>, tQueueConcurrency::MULTIPLE_READERS_FAST> queue;
    std::vector<std::thread> threads;

    for (int i = 0; i < cTHREADS; i++)
    {
      threads.emplace_back(DequeueBuffers<tQueueConcurrency::MULTIPLE_READERS_FAST>, std::ref(queue), false, 1);
    }
    EnqueueBuffers<tQueueConcurrency::MULTIPLE_READERS_FAST>(queue, buffers, 4);
    for (int i = 0; i < cTHREADS; i++)
    {
      threads[i].join();
    }

    if (dequeued_elements_global != cBUFFERS)
    {
      RRLIB_LOG_PRINT(ERROR, dequeued_elements_global, " elements were dequeued in total. Expected ", cBUFFERS);
      abort();
    }
  }

  // Test FULL_FAST
  {
    RRLIB_LOG_PRINT(USER, "Testing FULL_FAST implementation with ", cTHREADS, " reader and writer threads:");
    dequeued_elements_global = 0;
    tQueue<std::unique_ptr<tTestType>, tQueueConcurrency::FULL_FAST> queue;
    std::vector<std::thread> threads;

    for (int i = 0; i < cTHREADS; i++)
    {
      threads.emplace_back(EnqueueBuffers<tQueueConcurrency::FULL_FAST>, std::ref(queue), &(buffers[i * cBUFFERS]), i == 0 ? 6 : 5);
    }
    for (int i = 0; i < cTHREADS; i++)
    {
      threads.emplace_back(DequeueBuffers<tQueueConcurrency::FULL_FAST>, std::ref(queue), false, 5);
    }
    for (int i = 0; i < 2 * cTHREADS; i++)
    {
      threads[i].join();
    }

    if (dequeued_elements_global != cTHREADS * cBUFFERS)
    {
      RRLIB_LOG_PRINT(ERROR, dequeued_elements_global, " elements were dequeued in total. Expected ", (cTHREADS * cBUFFERS));
      abort();
    }
  }

  return 0;
}
