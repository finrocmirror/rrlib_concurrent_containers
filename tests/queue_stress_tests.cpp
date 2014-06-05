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
/*!\file    rrlib/concurrent_containers/tests/queue_stress_tests.cpp
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
#include "rrlib/util/tUnitTestSuite.h"

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
const int cTHREADS = 3;
const int cBUFFERS = 40000000;
//const int cBUFFERS = 1000000;
const int cWAIT_EVERY = 20000;

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------
std::atomic<int> dequeued_elements_global;
std::atomic<int> discarded_elements_global;
int test_count = 0;

class tTestType : public tQueueable<tQueueability::FULL>
{
public:
  int element_no;
  int8_t thread_no;
  bool terminator;
  std::atomic<int8_t> dequeued;
  std::atomic<int8_t> discarded;

  tTestType() : element_no(-1), thread_no(0), terminator(false), dequeued(0), discarded(0) {}

  tTestType(bool terminator) : element_no(-1), thread_no(0), terminator(true), dequeued(0), discarded(0) {}

  int GetThreadNo()
  {
    return thread_no;
  }
};

struct tCountingDeleter
{
  void operator()(tTestType* p) const
  {
    discarded_elements_global++;
    if (!p->terminator)
    {
      p->discarded++;
    }
    else
    {
      //RRLIB_LOG_PRINT(ERROR, "Discarded terminator");
    }
  }
};

template <typename T, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE, bool BOUNDED, bool WRITE_DELAYS>
void EnqueueBuffers(tQueue<T, CONCURRENCY, DEQUEUE_MODE, BOUNDED>& queue, tTestType* buffers)
{
  rrlib::time::tTimestamp start = rrlib::time::Now();
  for (int i = 0; i < cBUFFERS; i++, buffers++)
  {
    queue.Enqueue(T(buffers));
    if (WRITE_DELAYS && ((i + 1) % cWAIT_EVERY == 0))
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
  }

  RRLIB_LOG_PRINT(USER, "  Thread enqueued ", cBUFFERS, " elements in ", rrlib::time::ToString(rrlib::time::Now() - start), " starting at ", start, ".");
}

// for fifo queues
template <typename T, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE, bool BOUNDED>
struct tBufferDequeueing
{
  static void Dequeue(tQueue<T, CONCURRENCY, DEQUEUE_MODE, BOUNDED>& queue, int reader_threads)
  {
    int next_element_index[cTHREADS];
    memset(next_element_index, 0, sizeof(next_element_index));
    int dequeued_elements = 0;
    rrlib::time::tTimestamp start = rrlib::time::Now();

    while (true)
    {
      T buffer = queue.Dequeue();
      if (!buffer)
      {
        continue;
      }
      if (buffer->terminator)
      {
        delete buffer.release();
        dequeued_elements_global += dequeued_elements;
        RRLIB_LOG_PRINT(USER, "  Thread dequeued ", dequeued_elements, " elements in ", rrlib::time::ToString(rrlib::time::Now() - start), ".");
        return;
      }

      if (buffer->thread_no < 0 || buffer->thread_no >= cTHREADS)
      {
        RRLIB_LOG_PRINT(ERROR, "Invalid thread number: ", buffer->GetThreadNo());
        RRLIB_UNIT_TESTS_ASSERT_MESSAGE("Failed.", false);
      }

      if (reader_threads <= 1 && (!BOUNDED))
      {
        if (buffer->element_no != next_element_index[buffer->thread_no])
        {
          RRLIB_LOG_PRINT(ERROR, "Test failed: Element from thread ", buffer->GetThreadNo(), " - expected ", next_element_index[buffer->thread_no], " got ", buffer->element_no);
          RRLIB_LOG_PRINT(ERROR, "#Elements dequeued in total: ", dequeued_elements);
          RRLIB_UNIT_TESTS_ASSERT_MESSAGE("Failed.", false);
        }
        next_element_index[buffer->thread_no]++;
      }
      else
      {
        if (buffer->element_no < next_element_index[buffer->thread_no])
        {
          RRLIB_LOG_PRINT(ERROR, "Test failed: Element from thread ", buffer->GetThreadNo(), " - expected >= ", next_element_index[buffer->thread_no], " got ", buffer->element_no);
          RRLIB_LOG_PRINT(ERROR, "#Elements dequeued by this thread: ", dequeued_elements);
          RRLIB_UNIT_TESTS_ASSERT_MESSAGE("Failed.", false);
        }
        next_element_index[buffer->thread_no] = buffer->element_no + 1;
      }
      if (BOUNDED)
      {
        buffer->dequeued++;
      }
      buffer.release();
      dequeued_elements++;
    }
  }
};

template <typename T, tConcurrency CONCURRENCY, bool BOUNDED>
struct tBufferDequeueing<T, CONCURRENCY, tDequeueMode::ALL, BOUNDED>
{
  static void Dequeue(tQueue<T, CONCURRENCY, tDequeueMode::ALL, BOUNDED>& queue, int reader_threads)
  {
    int next_element_index[cTHREADS];
    memset(next_element_index, 0, sizeof(next_element_index));
    int dequeued_elements = 0;
    rrlib::time::tTimestamp start = rrlib::time::Now();

    while (true)
    {
      tQueueFragment<T> fragment = queue.DequeueAll();
      while (!fragment.Empty())
      {
        T buffer = fragment.PopFront();
        if (!buffer)
        {
          continue;
        }
        if (buffer->terminator)
        {
          delete buffer.release();
          dequeued_elements_global += dequeued_elements;
          RRLIB_LOG_PRINT(USER, "  Thread dequeued ", dequeued_elements, " elements in ", rrlib::time::ToString(rrlib::time::Now() - start), ".");
          RRLIB_UNIT_TESTS_ASSERT_MESSAGE("Failed.", false);
        }

        if (buffer->thread_no < 0 || buffer->thread_no >= cTHREADS)
        {
          RRLIB_LOG_PRINT(ERROR, "Invalid thread number: ", buffer->GetThreadNo());
          RRLIB_UNIT_TESTS_ASSERT_MESSAGE("Failed.", false);
        }

        if (reader_threads <= 1 && (!BOUNDED))
        {
          if (buffer->element_no != next_element_index[buffer->thread_no])
          {
            RRLIB_LOG_PRINT(ERROR, "Test failed: Element from thread ", buffer->GetThreadNo(), " - expected ", next_element_index[buffer->thread_no], " got ", buffer->element_no);
            RRLIB_UNIT_TESTS_ASSERT_MESSAGE("Failed.", false);
          }
          next_element_index[buffer->thread_no]++;
        }
        else
        {
          if (buffer->element_no < next_element_index[buffer->thread_no])
          {
            RRLIB_LOG_PRINT(ERROR, "Test failed: Element from thread ", buffer->GetThreadNo(), " - expected >= ", next_element_index[buffer->thread_no], " got ", buffer->element_no);
            RRLIB_UNIT_TESTS_ASSERT_MESSAGE("Failed.", false);
          }
          next_element_index[buffer->thread_no] = buffer->element_no + 1;
        }
        if (BOUNDED)
        {
          buffer->dequeued++;
        }
        buffer.release();
        dequeued_elements++;
      }
    }
  }
};


void BufferCheckAndReset(tTestType* buffers, int threads)
{
  for (int i = 0; i < threads; i++)
  {
    for (int j = 0; j < cBUFFERS; j++)
    {
      tTestType& tt = buffers[i * cBUFFERS + j];
      int deq = tt.dequeued.exchange(0);
      int dis = tt.discarded.exchange(0);
      if (!((deq == 1 && dis == 0) || (deq == 0 && dis == 1)))
      {
        RRLIB_LOG_PRINT(ERROR, "Corrupt element (index, ", i * cBUFFERS + j, ") - Dequeued: ", deq, "  Discarded: ", dis);
        RRLIB_UNIT_TESTS_ASSERT_MESSAGE("Failed.", false);
      }
      if (tt.next_queueable)
      {
        RRLIB_LOG_PRINT(ERROR, "Corrupt element (index, ", i * cBUFFERS + j, ") - next_queueable is not NULL");
        RRLIB_UNIT_TESTS_ASSERT_MESSAGE("Failed.", false);
      }
    }
  }
}

template <bool BOUNDED>
struct tMaxQueueLength
{
  template <typename Q>
  static void Set(Q& q, int max_len) {}
};

template <>
struct tMaxQueueLength<true>
{
  template <typename Q>
  static void Set(Q& q, int max_len)
  {
    q.SetMaxLength(max_len);
  }
};

template <tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE, int MAX_QUEUE_LENGTH, bool WRITE_DELAYS>
void PerformTest(tTestType* buffers)
{
  enum { MAX_QUEUE_LENGTH_SET = MAX_QUEUE_LENGTH != 0 };
  typedef typename std::conditional<MAX_QUEUE_LENGTH_SET, std::unique_ptr<tTestType, tCountingDeleter>, std::unique_ptr<tTestType>>::type tPointer;
  typedef tQueue<tPointer, CONCURRENCY, DEQUEUE_MODE, MAX_QUEUE_LENGTH_SET> tQueueType;

  test_count++;
  RRLIB_LOG_PRINTF(USER, "Test %d/96: tQueue<std::unique_ptr<tTestType>, tConcurrency::%s, tDequeueMode::%s, %d> %s:",
                   test_count, make_builder::GetEnumString(CONCURRENCY), make_builder::GetEnumString(DEQUEUE_MODE), MAX_QUEUE_LENGTH, WRITE_DELAYS ? "with write delay" : "without write delays");
  dequeued_elements_global = 0;
  discarded_elements_global = 0;
  const int cWRITER_THREADS = (CONCURRENCY == tConcurrency::MULTIPLE_WRITERS || CONCURRENCY == tConcurrency::FULL) ? cTHREADS : 1;
  const int cREADER_THREADS = (CONCURRENCY == tConcurrency::MULTIPLE_READERS || CONCURRENCY == tConcurrency::FULL) ? cTHREADS : 1;
  const int cTERMINATORS = cREADER_THREADS + tQueueType::cMINIMUM_ELEMENTS_IN_QEUEUE;

  tQueueType queue;
  tMaxQueueLength<MAX_QUEUE_LENGTH_SET>::Set(queue, MAX_QUEUE_LENGTH);
  std::vector<std::thread> threads;

  // create writer threads
  for (int i = 0; i < cWRITER_THREADS; i++)
  {
    threads.emplace_back(EnqueueBuffers<tPointer, CONCURRENCY, DEQUEUE_MODE, MAX_QUEUE_LENGTH_SET, WRITE_DELAYS>, std::ref(queue), &(buffers[i * cBUFFERS]));
  }

  // create reader threads
  for (int i = 0; i < cREADER_THREADS; i++)
  {
    threads.emplace_back(tBufferDequeueing<tPointer, CONCURRENCY, DEQUEUE_MODE, MAX_QUEUE_LENGTH_SET>::Dequeue, std::ref(queue), cREADER_THREADS);
  }

  // wait for writer threads
  for (int i = 0; i < cWRITER_THREADS; i++)
  {
    threads[i].join();
  }

  // send terminators
  for (int i = 0; i < cTERMINATORS; i++)
  {
    if (MAX_QUEUE_LENGTH || DEQUEUE_MODE == tDequeueMode::ALL)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    queue.Enqueue(tPointer(new tTestType(true)));
  }

  // wait for reader threads
  for (size_t i = cWRITER_THREADS; i < threads.size(); i++)
  {
    threads[i].join();
  }

  if (!MAX_QUEUE_LENGTH)
  {
    if (dequeued_elements_global != cWRITER_THREADS * cBUFFERS)
    {
      RRLIB_LOG_PRINT(ERROR, dequeued_elements_global, " elements were dequeued in total. Expected ", cWRITER_THREADS * cBUFFERS);
      RRLIB_UNIT_TESTS_ASSERT_MESSAGE("Failed.", false);
    }
  }
  else
  {
    BufferCheckAndReset(buffers, cWRITER_THREADS);
    int all = dequeued_elements_global + discarded_elements_global;
    if (all != cWRITER_THREADS * cBUFFERS)
    {
      RRLIB_LOG_PRINT(ERROR, all, " elements were dequeued/discarded in total. Expected ", cWRITER_THREADS * cBUFFERS);
      RRLIB_UNIT_TESTS_ASSERT_MESSAGE("Failed.", false);
    }
    RRLIB_LOG_PRINT(USER, dequeued_elements_global, " elements were dequeued in total. ", discarded_elements_global, " elements were discarded.");
  }
}

template <tDequeueMode DEQUEUE_MODE, int MAX_QUEUE_LENGTH, bool WRITE_DELAYS>
void PerformTests(tTestType* buffers)
{
  PerformTest<tConcurrency::SINGLE_READER_AND_WRITER, DEQUEUE_MODE, MAX_QUEUE_LENGTH, WRITE_DELAYS>(buffers);
  PerformTest<tConcurrency::MULTIPLE_WRITERS, DEQUEUE_MODE, MAX_QUEUE_LENGTH, WRITE_DELAYS>(buffers);
  PerformTest<tConcurrency::MULTIPLE_READERS, DEQUEUE_MODE, MAX_QUEUE_LENGTH, WRITE_DELAYS>(buffers);
  PerformTest<tConcurrency::FULL, DEQUEUE_MODE, MAX_QUEUE_LENGTH, WRITE_DELAYS>(buffers);
}

class QueueStressTest : public util::tUnitTestSuite
{
  RRLIB_UNIT_TESTS_BEGIN_SUITE(QueueStressTest);
  RRLIB_UNIT_TESTS_ADD_TEST(Test);
  RRLIB_UNIT_TESTS_END_SUITE;

  void Test()
  {
    RRLIB_LOG_PRINT(USER, "Allocating ", (cTHREADS * cBUFFERS * sizeof(tTestType)) / (1024 * 1024), " MB of buffers.");
    tTestType* buffers = new tTestType[cTHREADS * cBUFFERS];
    if (!buffers)
    {
      RRLIB_UNIT_TESTS_ASSERT_MESSAGE("Failed.", false);
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

    PerformTests<tDequeueMode::FIFO, 0, false>(buffers);
    PerformTests<tDequeueMode::FIFO_FAST, 0, false>(buffers);
    PerformTests<tDequeueMode::FIFO, 1, false>(buffers);
    PerformTests<tDequeueMode::FIFO_FAST, 1, false>(buffers);
    PerformTests<tDequeueMode::FIFO, 2, false>(buffers);
    PerformTests<tDequeueMode::FIFO_FAST, 2, false>(buffers);
    PerformTests<tDequeueMode::FIFO, 500, false>(buffers);
    PerformTests<tDequeueMode::FIFO_FAST, 500, false>(buffers);

    PerformTests<tDequeueMode::ALL, 0, false>(buffers);
    PerformTests<tDequeueMode::ALL, 1, false>(buffers);
    PerformTests<tDequeueMode::ALL, 2, false>(buffers);
    PerformTests<tDequeueMode::ALL, 500, false>(buffers);

    PerformTests<tDequeueMode::FIFO, 0, true>(buffers);
    PerformTests<tDequeueMode::FIFO_FAST, 0, true>(buffers);
    PerformTests<tDequeueMode::FIFO, 1, true>(buffers);
    PerformTests<tDequeueMode::FIFO_FAST, 1, true>(buffers);
    PerformTests<tDequeueMode::FIFO, 2, true>(buffers);
    PerformTests<tDequeueMode::FIFO_FAST, 2, true>(buffers);
    PerformTests<tDequeueMode::FIFO, 500, true>(buffers);
    PerformTests<tDequeueMode::FIFO_FAST, 500, true>(buffers);

    PerformTests<tDequeueMode::ALL, 0, true>(buffers);
    PerformTests<tDequeueMode::ALL, 1, true>(buffers);
    PerformTests<tDequeueMode::ALL, 2, true>(buffers);
    PerformTests<tDequeueMode::ALL, 500, true>(buffers);
  }

};

RRLIB_UNIT_TESTS_REGISTER_SUITE(QueueStressTest);

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
