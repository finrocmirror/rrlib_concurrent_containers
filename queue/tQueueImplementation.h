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
/*!\file    rrlib/concurrent_containers/queue/tQueueImplementation.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-09-25
 *
 * \brief   Contains tQueueImplementation
 *
 * \b tQueueImplementation
 *
 * Implementations for different types of queues.
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__queue__tQueueImplementation_h__
#define __rrlib__concurrent_containers__queue__tQueueImplementation_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/concurrent_containers/tQueueable.h"
#include "rrlib/concurrent_containers/tQueueableSingleThreaded.h"
#include "rrlib/concurrent_containers/queue/tUniquePtrQueueImplementation.h"

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
// Class declaration
//----------------------------------------------------------------------
//! Queue Implementation
/*!
 * Implementations for different types of queues.
 */
template <typename T, tQueueConcurrency CONCURRENCY>
class tQueueImplementation
{
  // this combination of parameters is not supported yet
};

template <typename T, tQueueConcurrency CONCURRENCY>
class tQueueImplementation<std::unique_ptr<T>, CONCURRENCY> : public tUniquePtrQueueImplementation<T, CONCURRENCY, std::is_base_of<tQueueable, T>::value, std::is_base_of<tQueueableSingleThreaded, T>::value>
{
  typedef tUniquePtrQueueImplementation<T, CONCURRENCY, std::is_base_of<tQueueable, T>::value, std::is_base_of<tQueueableSingleThreaded, T>::value> tBase;

protected:

//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
  inline std::unique_ptr<T> Dequeue(bool& success)
  {
    std::unique_ptr<T> ptr = tBase::Dequeue();
    success = ptr.get();
    return std::move(ptr);
  }
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif