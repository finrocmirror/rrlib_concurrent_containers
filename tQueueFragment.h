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
/*!\file    rrlib/concurrent_containers/tQueueFragment.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-13
 *
 * \brief   Contains tQueueFragment
 *
 * \b tQueueFragment
 *
 * Queue fragment: Set of queue elements obtained from queues with tDequeueMode::ALL.
 * Elements can be retrieved FIFO and LIFO.
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__tQueueFragment_h__
#define __rrlib__concurrent_containers__tQueueFragment_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/concurrent_containers/queue/tQueueFragmentImplementation.h"

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
//! Queue fragment
/*!
 * Queue fragment: Set of queue elements obtained from queues with tDequeueMode::ALL.
 * Elements can be retrieved FIFO and LIFO.
 *
 * \tparam T Enqueued elements. Identical, to parameter T of tQueue that fragment is obtained from.
 */
template <typename T>
class tQueueFragment : boost::noncopyable
{
  typedef queue::tQueueFragmentImplementation<T> tImplementation;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tQueueFragment() {}

  /*! Move constructor */
  tQueueFragment(tQueueFragment && other)
  {
    std::swap(implementation, other.implementation);
  }

  /*! Constructor used by queue implementations (typically not to be called by client code) */
  tQueueFragment(tImplementation && implementation)
  {
    std::swap(this->implementation, implementation);
  }

  /*! Move assignment */
  tQueueFragment& operator=(tQueueFragment && other)
  {
    std::swap(implementation, other.implementation);
    return *this;
  }

  /*!
   * \return True, if there are no elements (left) in this fragment.
   */
  bool Empty()
  {
    return implementation.Empty();
  }

  /*!
   * Returns and removes the element from queue fragment that was enqueued first
   * (the first call possibly involves reverting the element order => a little overhead)
   *
   * \return Element that was removed
   */
  T PopFront()
  {
    return implementation.PopFront();
  }

  /*!
   * Returns and removes the element from queue fragment that was enqueued last
   * (the first call possibly involves reverting the element order => a little overhead)
   *
   * \return Element that was removed
   */
  T PopBack()
  {
    return implementation.PopBack();
  }

  /*!
   * Returns and removes an element from the queue fragment
   *
   * \return Element that was removed
   */
  T PopAny()
  {
    return implementation.PopAny();
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! queue fragment implementation */
  tImplementation implementation;

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
