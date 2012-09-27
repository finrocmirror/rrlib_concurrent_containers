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
/*!\file    rrlib/concurrent_containers/tQueueableSingleThreaded.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-09-26
 *
 * \brief   Contains tQueueableSingleThreaded
 *
 * \b tQueueableSingleThreaded
 *
 * This is the base class of queueable objects.
 * It contains the pointer to the next element in singly-linked, non-concurrent queue.
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__tQueueableSingleThreaded_h__
#define __rrlib__concurrent_containers__tQueueableSingleThreaded_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <boost/utility.hpp>

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/concurrent_containers/tQueueConcurrency.h"

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
namespace queue
{
template <typename T, tQueueConcurrency CONCURRENCY, bool QUEUEABLE_TYPE, bool QUEUEABLE_SINGLE_THREADED_TYPE>
class tUniquePtrQueueImplementation;
}

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Base class for queueable object
/*!
 * This is the base class of queueable objects.
 * It contains the pointer to the next element in singly-linked, non-concurrent queue.
 *
 * If objects are used in single-threaded queues only, you should derive
 * from this class instead from tQueueable - as this is more efficient.
 *
 * If objects are used in single-threaded queues also, you may derive from this
 * class additionally - as this is more efficient
 * (computationally, object size will slightly increase though).
 */
class tQueueableSingleThreaded : boost::noncopyable
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tQueueableSingleThreaded();

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  template <typename T, tQueueConcurrency CONCURRENCY, bool QUEUEABLE_TYPE, bool QUEUEABLE_SINGLE_THREADED_TYPE>
  friend class queue::tUniquePtrQueueImplementation;

  /*!
   * Pointer to next element in reuse queue... null if there's none
   * Does not need to be volatile... because only critical for reader
   * thread regarding terminator/null (and reader thread sets this himself)...
   * writer changes may be delayed without problem
   */
  tQueueableSingleThreaded* next_single_threaded_queueable;

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
