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
/*!\file    rrlib/concurrent_containers/queue/tIntrusiveQueueFragment.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-13
 *
 */
//----------------------------------------------------------------------
#include "rrlib/concurrent_containers/queue/tIntrusiveQueueFragment.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

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
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------
void tIntrusiveQueueFragmentQueueable::Turn()
{
  assert((!fifo_order || trim_to_size < 0) && "This only works under these conditions");
  tQueueableMost* first = PopAny();
  tQueueableMost* current = first;
  tQueueableMost* next = PopAny();
  while (next)
  {
    tQueueableMost* prev = current;
    current = next;
    next = PopAny();
    current->next_queueable = prev;
  }

  to_delete = next_queueable; // any remaining
  next_queueable = current;
  fifo_order = !fifo_order;
  trim_to_size = -1;
}

void tIntrusiveQueueFragmentQueueableSingleThreaded::Turn()
{
  tQueueableSingleThreaded* first = PopAny();
  tQueueableSingleThreaded* current = first;
  tQueueableSingleThreaded* next = PopAny();
  while (next)
  {
    tQueueableSingleThreaded* prev = current;
    current = next;
    next = PopAny();
    current->next_single_threaded_queueable = prev;
  }
  InitSingleThreaded(current, !fifo_order_single_threaded);
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
