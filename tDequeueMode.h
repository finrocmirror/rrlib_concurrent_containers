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
/*!\file    rrlib/concurrent_containers/tDequeueMode.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-13
 *
 * \brief   Contains tDequeueMode
 *
 * \b tDequeueMode
 *
 * Different options for how elements can be dequeued from a queue.
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__tDequeueMode_h__
#define __rrlib__concurrent_containers__tDequeueMode_h__

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

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Queue Dequeue Mode
/*!
 * Different options for how elements can be dequeued from a queue.
 */
enum class tDequeueMode
{
  /*!
   * Single elements are dequeued (first in first out)
   */
  FIFO,

  /*!
   * Single elements are dequeued (first in first out). Using this mode, the last element in the queue might not be dequeueable -
   * making the queue always contain at least one element (can be determined via tQueue::cMINIMUM_ELEMENTS_IN_QEUEUE).
   * Such queues are more efficient with respect to computational overhead.
   */
  FIFO_FAST,

  /*!
   * All elements are dequeued at once and returned in a tQueueFragment.
   * This is typically the most efficient in concurrent implementations.
   */
  ALL
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
