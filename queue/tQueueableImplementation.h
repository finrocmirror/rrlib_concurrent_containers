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
/*!\file    rrlib/concurrent_containers/queue/tQueueableImplementation.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-14
 *
 * \brief   Contains tQueueableImplementation
 *
 * \b tQueueableImplementation
 *
 * Implementation for tQueueable with different levels of queueability.
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__queue__tQueueableImplementation_h__
#define __rrlib__concurrent_containers__queue__tQueueableImplementation_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Namespace declaration
//----------------------------------------------------------------------
namespace rrlib
{
namespace concurrent_containers
{
namespace queue
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

class tQueueableMost : boost::noncopyable
{
//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tQueueableMost();

  /*!
   * Pointer to next element in queue... null if there's none
   *
   * Needs to be atomic - anything else would not really be clean
   * (unfortunately, this really hurts performance - almost factor 10).
   * Queue stress test fails with non-atomic pointer with multiple writer threads.
   */
  std::atomic<tQueueableMost*> next_queueable;

  /*! Terminator (not null for efficiency reasons) */
  static tQueueableMost terminator;

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  tQueueableMost(bool terminator);
};

class tQueueableFull : public tQueueableMost
{
//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tQueueableFull();

  /*!
   * Atomic Pointer to an element in queue.
   * Required in some queues.
   */
  union
  {
    std::atomic<tQueueableFull*> queueable_pointer;
    std::atomic<size_t> queueable_tagged_pointer;
  };
};


class tQueueableSingleThreaded : boost::noncopyable
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tQueueableSingleThreaded();

  /*!
   * Pointer to next element in reuse queue... null if there's none
   * Does not need to be volatile... because only critical for reader
   * thread regarding terminator/null (and reader thread sets this himself)...
   * writer changes may be delayed without problem
   */
  tQueueableSingleThreaded* next_single_threaded_queueable;
};

} // namespace

template<> class tQueueable<tQueueability::SINGLE_THREADED> : public queue::tQueueableSingleThreaded {};
template<> class tQueueable<tQueueability::MOST> : public queue::tQueueableMost {};
template<> class tQueueable<tQueueability::MOST_OPTIMIZED> : public queue::tQueueableMost, public queue::tQueueableSingleThreaded {};
template<> class tQueueable<tQueueability::FULL> : public queue::tQueueableFull {};
template<> class tQueueable<tQueueability::FULL_OPTIMIZED> : public queue::tQueueableFull, public queue::tQueueableSingleThreaded {};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
