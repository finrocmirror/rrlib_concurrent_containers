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
struct tFirstBaseClass
{
  int64_t an_integer;
};

template <tQueueability QA>
struct tTestType : public tFirstBaseClass, public tQueueable<QA>
{
  int value;

  tTestType(int value) : value(value) {}

  bool operator==(const tTestType& other)
  {
    return value == other.value;
  }
};

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

template <typename Q, typename REFQ>
void DequeueElement(Q& queue, REFQ& ref_queue)
{
  typename Q::tElement ptr1;
  typename Q::tElement ptr2;
  DequeueElement(queue, ref_queue, ptr1, ptr2);
}

template <typename Q, typename REFQ, typename PTR>
void DequeueElement(Q& queue, REFQ& ref_queue, PTR& qptr, PTR& refqptr)
{
  bool success = false;
  qptr = queue.Dequeue(success);
  assert(success == (qptr.get() != NULL) && "Setting success seems broken");
  refqptr = ref_queue.Dequeue(success);
  if (qptr)
  {
    RRLIB_LOG_PRINT(USER, "  Dequeued ", qptr->value);
    if (!refqptr)
    {
      RRLIB_LOG_PRINT(ERROR, "  Should be empty");
      abort();
    }
    else if (!(*qptr == *refqptr))
    {
      RRLIB_LOG_PRINT(ERROR, "  Expected ", refqptr->value);
      abort();
    }
  }
  else
  {
    RRLIB_LOG_PRINT(USER, "  Dequeued nothing");
  }
}

template <typename Q, typename REFQ>
void DequeueAll(Q& queue, REFQ& ref_queue, bool fifo, int count, bool reenqueue)
{
  tQueueFragment<typename Q::tElement> fragment = queue.DequeueAll();
  tQueueFragment<typename Q::tElement> ref_fragment = ref_queue.DequeueAll();
  for (int i = 0; i < count; ++i)
  {
    typename Q::tElement qptr = fifo ? fragment.PopFront() : fragment.PopBack();
    typename Q::tElement refqptr = fifo ? ref_fragment.PopFront() : ref_fragment.PopBack();

    if (qptr)
    {
      RRLIB_LOG_PRINT(USER, "  Dequeued ", qptr->value);
      if (!refqptr)
      {
        RRLIB_LOG_PRINT(ERROR, "  Should be empty");
        abort();
      }
      else if (!(*qptr == *refqptr))
      {
        RRLIB_LOG_PRINT(ERROR, "  Expected ", refqptr->value);
        abort();
      }
    }
    else
    {
      RRLIB_LOG_PRINT(USER, "  Dequeued nothing");
      if (refqptr)
      {
        RRLIB_LOG_PRINT(ERROR, "  Expected ", refqptr->value);
        abort();
      }
    }

    if (reenqueue && qptr && refqptr)
    {
      queue.Enqueue(qptr);
      ref_queue.Enqueue(refqptr);
    }
  }
}

template <typename T, int MIN_SIZE, bool BOUNDED>
struct tRefQueueType
{
  typedef tQueue<std::unique_ptr<T>, tConcurrency::NONE, tDequeueMode::FIFO, BOUNDED> type;
};

template <typename T, bool BOUNDED>
struct tRefQueueType<T, 1, BOUNDED>
{
  typedef tQueue<std::unique_ptr<T>, tConcurrency::SINGLE_READER_AND_WRITER, tDequeueMode::FIFO_FAST, BOUNDED> type;
  static_assert(type::cMINIMUM_ELEMENTS_IN_QEUEUE == 1, "We need another ref queue type");
};

template <tConcurrency CONCURRENCY, tDequeueMode DQMODE, int MAX_QUEUE_LENGTH, tQueueability QA>
void TestQueue()
{
  typedef ::tTestType<QA> tTestType;
  RRLIB_LOG_PRINTF(USER, "Testing tQueue<std::unique_ptr<tTestType>, tConcurrency::%s, tDequeueMode::%s, %d> with tQueueable<%s>",
                   make_builder::GetEnumString(CONCURRENCY), make_builder::GetEnumString(DQMODE), MAX_QUEUE_LENGTH, make_builder::GetEnumString(QA));
  typedef tQueue < std::unique_ptr<tTestType>, CONCURRENCY, DQMODE, MAX_QUEUE_LENGTH != 0 > tQueueType;
  typedef typename tRefQueueType < tTestType, tQueueType::cMINIMUM_ELEMENTS_IN_QEUEUE, MAX_QUEUE_LENGTH != 0 >::type tRefQueueType;

  tQueueType q;
  tMaxQueueLength < MAX_QUEUE_LENGTH != 0 >::Set(q, MAX_QUEUE_LENGTH);
  tRefQueueType ref_q;
  tMaxQueueLength < MAX_QUEUE_LENGTH != 0 >::Set(ref_q, MAX_QUEUE_LENGTH);

  RRLIB_LOG_PRINT(USER, " Dequeueing two elements:");
  for (int i = 0; i < 2; i++)
  {
    DequeueElement(q, ref_q);
  }

  RRLIB_LOG_PRINT(USER, " Enqueueing ten elements: 1 to 10");
  for (int i = 1; i <= 10; i++)
  {
    q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i)));
    ref_q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i)));
  }
  RRLIB_LOG_PRINT(USER, " Dequeueing twelve elements:");
  for (int i = 0; i < 12; i++)
  {
    DequeueElement(q, ref_q);
  }

  RRLIB_LOG_PRINT(USER, " Enqueueing ten elements: 11 to 20");
  for (int i = 11; i <= 20; i++)
  {
    q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i)));
    ref_q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i)));
  }
  RRLIB_LOG_PRINT(USER, " Dequeueing five elements and enqueueing them again:");
  for (int i = 0; i < 5; i++)
  {
    std::unique_ptr<tTestType> ptr;
    std::unique_ptr<tTestType> ref_ptr;
    DequeueElement(q, ref_q, ptr, ref_ptr);
    q.Enqueue(ptr);
    ref_q.Enqueue(ref_ptr);
  }
  RRLIB_LOG_PRINT(USER, " Dequeueing twelve elements:");
  for (int i = 0; i < 12; i++)
  {
    DequeueElement(q, ref_q);
  }

  RRLIB_LOG_PRINT(USER, " Performing one enqueue and dequeue operation 5 times (elements 100 to 104):");
  for (int i = 0; i < 5; i++)
  {
    q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i + 100)));
    ref_q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i + 100)));
    DequeueElement(q, ref_q);
  }

  RRLIB_LOG_PRINT(USER, " ");
}

template <tConcurrency CONCURRENCY, int MAX_QUEUE_LENGTH, tQueueability QA>
void TestFragmentQueue()
{
  typedef ::tTestType<QA> tTestType;
  RRLIB_LOG_PRINTF(USER, "Testing tQueue<std::unique_ptr<tTestType>, tConcurrency::%s, tDequeueMode::ALL, %d> with tQueueable<%s>",
                   make_builder::GetEnumString(CONCURRENCY), MAX_QUEUE_LENGTH, make_builder::GetEnumString(QA));
  tQueue < std::unique_ptr<tTestType>, CONCURRENCY, tDequeueMode::ALL, MAX_QUEUE_LENGTH != 0 > q;
  tMaxQueueLength < MAX_QUEUE_LENGTH != 0 >::Set(q, MAX_QUEUE_LENGTH);
  tQueue < std::unique_ptr<tTestType>, tConcurrency::NONE, tDequeueMode::ALL, MAX_QUEUE_LENGTH != 0 > ref_q;
  tMaxQueueLength < MAX_QUEUE_LENGTH != 0 >::Set(ref_q, MAX_QUEUE_LENGTH);

  RRLIB_LOG_PRINT(USER, " Dequeueing two elements from dequeued fragment:");
  DequeueAll(q, ref_q, true, 2, false);

  RRLIB_LOG_PRINT(USER, " Enqueueing ten elements: 1 to 10");
  for (int i = 1; i <= 10; i++)
  {
    q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i)));
    ref_q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i)));
  }
  RRLIB_LOG_PRINT(USER, " PopFront() twelve elements from dequeued fragment:");
  DequeueAll(q, ref_q, true, 12, false);
  RRLIB_LOG_PRINT(USER, " PopFront() two elements from another dequeued fragment:");
  DequeueAll(q, ref_q, true, 2, false);

  RRLIB_LOG_PRINT(USER, " Enqueueing ten elements: 11 to 20");
  for (int i = 11; i <= 20; i++)
  {
    q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i)));
    ref_q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i)));
  }
  RRLIB_LOG_PRINT(USER, " PopBack() five elements and enqueueing them again:");
  DequeueAll(q, ref_q, false, 5, true);
  RRLIB_LOG_PRINT(USER, " PopBack() six elements from next fragment:");
  DequeueAll(q, ref_q, false, 6, false);

  RRLIB_LOG_PRINT(USER, " Performing one enqueue and dequeue operation 5 times (elements 100 to 104):");
  for (int i = 0; i < 5; i++)
  {
    q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i + 100)));
    ref_q.Enqueue(std::unique_ptr<tTestType>(new tTestType(i + 100)));
    DequeueAll(q, ref_q, i % 2, 1, false);
  }

  RRLIB_LOG_PRINT(USER, " ");
}

template <tDequeueMode DEQUEUE_MODE, int MAX_QUEUE_LENGTH>
void TestQueueConcurrencyLevels()
{
  TestQueue<tConcurrency::NONE, DEQUEUE_MODE, MAX_QUEUE_LENGTH, tQueueability::SINGLE_THREADED>();
  TestQueue<tConcurrency::NONE, DEQUEUE_MODE, MAX_QUEUE_LENGTH, tQueueability::MOST>();
  TestQueue<tConcurrency::NONE, DEQUEUE_MODE, MAX_QUEUE_LENGTH, tQueueability::FULL_OPTIMIZED>();
  TestQueue<tConcurrency::SINGLE_READER_AND_WRITER, DEQUEUE_MODE, MAX_QUEUE_LENGTH, tQueueability::MOST>();
  TestQueue<tConcurrency::SINGLE_READER_AND_WRITER, DEQUEUE_MODE, MAX_QUEUE_LENGTH, tQueueability::FULL_OPTIMIZED>();
  TestQueue<tConcurrency::MULTIPLE_WRITERS, DEQUEUE_MODE, MAX_QUEUE_LENGTH, tQueueability::MOST>();
  TestQueue<tConcurrency::MULTIPLE_WRITERS, DEQUEUE_MODE, MAX_QUEUE_LENGTH, tQueueability::FULL_OPTIMIZED>();
  TestQueue<tConcurrency::MULTIPLE_READERS, DEQUEUE_MODE, MAX_QUEUE_LENGTH, tQueueability::MOST>();
  TestQueue<tConcurrency::MULTIPLE_READERS, DEQUEUE_MODE, MAX_QUEUE_LENGTH, tQueueability::FULL_OPTIMIZED>();
  TestQueue<tConcurrency::FULL, DEQUEUE_MODE, MAX_QUEUE_LENGTH, tQueueability::MOST>();
  TestQueue<tConcurrency::FULL, DEQUEUE_MODE, MAX_QUEUE_LENGTH, tQueueability::FULL_OPTIMIZED>();
}

template <int MAX_QUEUE_LENGTH, tQueueability BASIC>
void TestFragmentQueueConcurrencyLevels()
{
  TestFragmentQueue<tConcurrency::NONE, MAX_QUEUE_LENGTH, tQueueability::SINGLE_THREADED>();
  TestFragmentQueue<tConcurrency::NONE, MAX_QUEUE_LENGTH, tQueueability::MOST>();
  TestFragmentQueue<tConcurrency::NONE, MAX_QUEUE_LENGTH, tQueueability::FULL_OPTIMIZED>();
  TestFragmentQueue<tConcurrency::SINGLE_READER_AND_WRITER, MAX_QUEUE_LENGTH, BASIC>();
  TestFragmentQueue<tConcurrency::SINGLE_READER_AND_WRITER, MAX_QUEUE_LENGTH, tQueueability::FULL_OPTIMIZED>();
  TestFragmentQueue<tConcurrency::MULTIPLE_WRITERS, MAX_QUEUE_LENGTH, BASIC>();
  TestFragmentQueue<tConcurrency::MULTIPLE_WRITERS, MAX_QUEUE_LENGTH, tQueueability::FULL_OPTIMIZED>();
  TestFragmentQueue<tConcurrency::MULTIPLE_READERS, MAX_QUEUE_LENGTH, BASIC>();
  TestFragmentQueue<tConcurrency::MULTIPLE_READERS, MAX_QUEUE_LENGTH, tQueueability::FULL_OPTIMIZED>();
  TestFragmentQueue<tConcurrency::FULL, MAX_QUEUE_LENGTH, BASIC>();
  TestFragmentQueue<tConcurrency::FULL, MAX_QUEUE_LENGTH, tQueueability::FULL_OPTIMIZED>();
}

int main(int, char**)
{
  TestQueueConcurrencyLevels<tDequeueMode::FIFO, 0>();
  TestQueueConcurrencyLevels<tDequeueMode::FIFO_FAST, 0>();
  TestQueueConcurrencyLevels<tDequeueMode::FIFO, 1>();
  TestQueueConcurrencyLevels<tDequeueMode::FIFO_FAST, 1>();
  TestQueueConcurrencyLevels<tDequeueMode::FIFO, 2>();
  TestQueueConcurrencyLevels<tDequeueMode::FIFO_FAST, 2>();
  TestQueueConcurrencyLevels<tDequeueMode::FIFO, 5>();
  TestQueueConcurrencyLevels<tDequeueMode::FIFO_FAST, 5>();

  TestFragmentQueueConcurrencyLevels<0, tQueueability::MOST>();
  TestFragmentQueueConcurrencyLevels<1, tQueueability::FULL>();
  TestFragmentQueueConcurrencyLevels<2, tQueueability::FULL>();
  TestFragmentQueueConcurrencyLevels<5, tQueueability::FULL>();

  return 0;
}
