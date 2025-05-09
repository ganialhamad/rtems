/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup POSIXAPI
 *
 * @brief Function Starts a New Thread in The Calling Process
 */

/*
 *  16.1.2 Thread Creation, P1003.1c/Draft 10, p. 144
 */

/*
 *  COPYRIGHT (c) 1989-2014.
 *  On-Line Applications Research Corporation (OAR).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pthread.h>
#include <errno.h>

#include <rtems/posix/posixapi.h>
#include <rtems/posix/priorityimpl.h>
#if defined(RTEMS_POSIX_API)
#include <rtems/posix/psignalimpl.h>
#endif
#include <rtems/posix/pthreadimpl.h>
#include <rtems/posix/pthreadattrimpl.h>
#include <rtems/score/assert.h>
#include <rtems/score/threadcpubudget.h>
#include <rtems/score/threadimpl.h>
#include <rtems/score/apimutex.h>
#include <rtems/score/stackimpl.h>
#include <rtems/score/statesimpl.h>
#include <rtems/score/schedulerimpl.h>
#include <rtems/score/userextimpl.h>
#include <rtems/sysinit.h>

#include <string.h>

static inline size_t _POSIX_Threads_Ensure_minimum_stack (
  size_t size
)
{
  if ( size >= PTHREAD_MINIMUM_STACK_SIZE )
    return size;
  return PTHREAD_MINIMUM_STACK_SIZE;
}


int pthread_create(
  pthread_t              *thread,
  const pthread_attr_t   *attr,
  void                 *(*start_routine)( void * ),
  void                   *arg
)
{
  Thread_Entry_information entry = {
    .adaptor = _Thread_Entry_adaptor_pointer,
    .Kinds = {
      .Pointer = {
        .entry = start_routine,
        .argument = arg
      }
    }
  };
  const pthread_attr_t               *the_attr;
  int                                 normal_prio;
  bool                                valid;
  Thread_Configuration                config;
  Status_Control                      status;
  Thread_Control                     *the_thread;
  Thread_Control                     *executing;
  int                                 schedpolicy = SCHED_RR;
  struct sched_param                  schedparam;
  int                                 error;
  ISR_lock_Context                    lock_context;
#if defined(RTEMS_POSIX_API)
  int                                 low_prio;
  Priority_Control                    core_low_prio;
  POSIX_API_Control                  *api;
#endif

  if ( !start_routine )
    return EFAULT;

  the_attr = (attr) ? attr : &_POSIX_Threads_Default_attributes;

  if ( !the_attr->is_initialized )
    return EINVAL;

  memset( &config, 0, sizeof( config ) );

  /*
   *  Currently all POSIX threads are floating point if the hardware
   *  supports it.
   */
  config.is_fp = true;

  config.is_preemptible = true;

  /*
   *  Core Thread Initialize ensures we get the minimum amount of
   *  stack space if it is allowed to allocate it itself.
   *
   *  NOTE: If the user provides the stack we will let it drop below
   *        twice the minimum.
   */
  if ( the_attr->stackaddr != NULL ) {
    if ( !_Stack_Is_enough( the_attr->stacksize, config.is_fp ) ) {
      return EINVAL;
    }

    config.stack_area = the_attr->stackaddr;
    config.stack_size = the_attr->stacksize;
  } else {
    config.stack_size = _POSIX_Threads_Ensure_minimum_stack(
      the_attr->stacksize
    );
    config.stack_size = _Stack_Extend_size( config.stack_size, config.is_fp );
  }

  #if 0
    int  cputime_clock_allowed;  /* see time.h */
    rtems_set_errno_and_return_minus_one( ENOSYS );
  #endif

  executing = _Thread_Get_executing();

  /*
   *  P1003.1c/Draft 10, p. 121.
   *
   *  If inheritsched is set to PTHREAD_INHERIT_SCHED, then this thread
   *  inherits scheduling attributes from the creating thread.   If it is
   *  PTHREAD_EXPLICIT_SCHED, then scheduling parameters come from the
   *  attributes structure.
   */
  switch ( the_attr->inheritsched ) {
    case PTHREAD_INHERIT_SCHED:
      error = pthread_getschedparam(
        pthread_self(),
        &schedpolicy,
        &schedparam
      );
      _Assert( error == 0 );
      (void) error; /* error only used when debug enabled */
      break;

    case PTHREAD_EXPLICIT_SCHED:
      schedpolicy = the_attr->schedpolicy;
      schedparam  = the_attr->schedparam;
      break;

    default:
      return EINVAL;
  }

  /*
   *  Check the contentionscope since rtems only supports PROCESS wide
   *  contention (i.e. no system wide contention).
   */
  if ( the_attr->contentionscope != PTHREAD_SCOPE_PROCESS )
    return ENOTSUP;

  error = _POSIX_Thread_Translate_sched_param(
    schedpolicy,
    &schedparam,
    &config
  );
  if ( error != 0 ) {
    return error;
  }

  normal_prio = schedparam.sched_priority;

  config.scheduler = _Thread_Scheduler_get_home( executing );

  config.priority = _POSIX_Priority_To_core(
    config.scheduler,
    normal_prio,
    &valid
  );
  if ( !valid ) {
    return EINVAL;
  }

#if defined(RTEMS_POSIX_API)
  if ( schedpolicy == SCHED_SPORADIC ) {
    low_prio = schedparam.sched_ss_low_priority;
  } else {
    low_prio = normal_prio;
  }

  core_low_prio = _POSIX_Priority_To_core( config.scheduler, low_prio, &valid );
  if ( !valid ) {
    return EINVAL;
  }
#endif

  if ( the_attr->affinityset == NULL ) {
    return EINVAL;
  }

  /*
   *  Allocate the thread control block.
   *
   *  NOTE:  Global threads are not currently supported.
   */
  the_thread = _POSIX_Threads_Allocate();
  if ( !the_thread ) {
    _Objects_Allocator_unlock();
    return EAGAIN;
  }

  if ( config.stack_area == NULL ) {
    config.stack_free = _Stack_Free;
    config.stack_area = _Stack_Allocate( config.stack_size );
  } else {
    config.stack_free = _Objects_Free_nothing;
  }

  if ( config.stack_area == NULL ) {
    _Objects_Free( &_POSIX_Threads_Information.Objects, &the_thread->Object );
    _Objects_Allocator_unlock();
    return EAGAIN;
  }

  /*
   *  Initialize the core thread for this task.
   */
  status = _Thread_Initialize(
    &_POSIX_Threads_Information,
    the_thread,
    &config
  );
  if ( status != STATUS_SUCCESSFUL ) {
    _Objects_Allocator_unlock();
    return _POSIX_Get_error( status );
  }

  if ( the_attr->detachstate == PTHREAD_CREATE_DETACHED ) {
    the_thread->Life.state |= THREAD_LIFE_DETACHED;
  }

  the_thread->Life.state |= THREAD_LIFE_CHANGE_DEFERRED;

  _ISR_lock_ISR_disable( &lock_context );
   status = _Scheduler_Set_affinity(
     the_thread,
     the_attr->affinitysetsize,
     the_attr->affinityset
   );
  _ISR_lock_ISR_enable( &lock_context );
   if ( status != STATUS_SUCCESSFUL ) {
      _Thread_Free( &_POSIX_Threads_Information, the_thread );
     _RTEMS_Unlock_allocator();
     return EINVAL;
   }

  the_thread->was_created_with_inherited_scheduler =
    ( the_attr->inheritsched == PTHREAD_INHERIT_SCHED );

#if defined(RTEMS_POSIX_API)
  /*
   *  finish initializing the per API structure
   */
  api = the_thread->API_Extensions[ THREAD_API_POSIX ];

  _Priority_Node_set_priority( &api->Sporadic.Low_priority, core_low_prio );
  api->Sporadic.sched_ss_repl_period =
    the_attr->schedparam.sched_ss_repl_period;
  api->Sporadic.sched_ss_init_budget =
    the_attr->schedparam.sched_ss_init_budget;
  api->Sporadic.sched_ss_max_repl =
    the_attr->schedparam.sched_ss_max_repl;

  if ( schedpolicy == SCHED_SPORADIC ) {
#if defined(RTEMS_SCORE_THREAD_HAS_SCHEDULER_CHANGE_INHIBITORS)
    the_thread->is_scheduler_change_inhibited = true;
#endif
    _POSIX_Threads_Sporadic_timer( &api->Sporadic.Timer );
  }
#endif

  /*
   *  POSIX threads are allocated and started in one operation.
   */
  _ISR_lock_ISR_disable( &lock_context );
  status = _Thread_Start( the_thread, &entry, &lock_context );

  #if defined(RTEMS_DEBUG)
    /*
     *  _Thread_Start only fails if the thread was in the incorrect state
     *
     *  NOTE: This can only happen if someone slips in and touches the
     *        thread while we are creating it.
     */
    if ( status != STATUS_SUCCESSFUL ) {
      _Thread_Free( &_POSIX_Threads_Information, the_thread );
      _Objects_Allocator_unlock();
      return EINVAL;
    }
  #endif

  /*
   *  Return the id and indicate we successfully created the thread
   */
  *thread = the_thread->Object.id;

  _Objects_Allocator_unlock();
  return 0;
}

#if defined(RTEMS_POSIX_API)
void _POSIX_Threads_Sporadic_timer( Watchdog_Control *watchdog )
{
  POSIX_API_Control    *api;
  Thread_Control       *the_thread;
  Thread_queue_Context  queue_context;

  api = RTEMS_CONTAINER_OF( watchdog, POSIX_API_Control, Sporadic.Timer );
  the_thread = api->Sporadic.thread;

  _Thread_queue_Context_initialize( &queue_context );
  _Thread_queue_Context_clear_priority_updates( &queue_context );
  _Thread_Wait_acquire( the_thread, &queue_context );

  if ( !_Priority_Node_is_active( &the_thread->Real_priority ) ) {
    _Thread_Priority_add(
      the_thread,
      &the_thread->Real_priority,
      &queue_context
    );
    _Thread_Priority_remove(
      the_thread,
      &api->Sporadic.Low_priority,
      &queue_context
    );
  }

  _Watchdog_Per_CPU_remove_ticks( &api->Sporadic.Timer );
  _POSIX_Threads_Sporadic_timer_insert( the_thread, api );

  _Thread_Wait_release( the_thread, &queue_context );
  _Thread_Priority_update( &queue_context );
}

static void _POSIX_Threads_Sporadic_budget_callout(
  Thread_Control *the_thread
)
{
  POSIX_API_Control    *api;
  Thread_queue_Context  queue_context;

  api = the_thread->API_Extensions[ THREAD_API_POSIX ];

  _Thread_queue_Context_initialize( &queue_context );
  _Thread_queue_Context_clear_priority_updates( &queue_context );
  _Thread_Wait_acquire( the_thread, &queue_context );

  /*
   *  This will prevent the thread from consuming its entire "budget"
   *  while at low priority.
   */
  the_thread->CPU_budget.available = UINT32_MAX;

  if ( _Priority_Node_is_active( &the_thread->Real_priority ) ) {
    _Thread_Priority_add(
      the_thread,
      &api->Sporadic.Low_priority,
      &queue_context
    );
    _Thread_Priority_remove(
      the_thread,
      &the_thread->Real_priority,
      &queue_context
    );
    _Priority_Node_set_inactive( &the_thread->Real_priority );
  }

  _Thread_Wait_release( the_thread, &queue_context );
  _Thread_Priority_update( &queue_context );
}

static void _POSIX_Threads_Sporadic_budget_at_tick( Thread_Control *the_thread )
{
  uint32_t budget_available;

  if ( !the_thread->is_preemptible ) {
    return;
  }

  if ( !_States_Is_ready( the_thread->current_state ) ) {
    return;
  }

  budget_available = the_thread->CPU_budget.available;

  if ( budget_available == 1 ) {
    the_thread->CPU_budget.available = 0;
    _POSIX_Threads_Sporadic_budget_callout ( the_thread );
  } else {
    the_thread->CPU_budget.available = budget_available - 1;
  }
}

const Thread_CPU_budget_operations _POSIX_Threads_Sporadic_budget = {
  .at_tick = _POSIX_Threads_Sporadic_budget_at_tick,
  .at_context_switch = _Thread_CPU_budget_do_nothing,
  .initialize = _Thread_CPU_budget_do_nothing
};

static bool _POSIX_Threads_Create_extension(
  Thread_Control *executing,
  Thread_Control *created
)
{
  POSIX_API_Control *api;

  api = created->API_Extensions[ THREAD_API_POSIX ];

  api->Sporadic.thread = created;
  _Watchdog_Preinitialize( &api->Sporadic.Timer, _Per_CPU_Get_by_index( 0 ) );
  _Watchdog_Initialize( &api->Sporadic.Timer, _POSIX_Threads_Sporadic_timer );
  _Priority_Node_set_inactive( &api->Sporadic.Low_priority );

#if defined(RTEMS_POSIX_API)
  /*
   * There are some subtle rules which need to be followed for
   * the value of the created thread's signal mask. Because signals
   * are part of C99 and enhanced by POSIX, both Classic API tasks
   * and POSIX threads have to have them enabled.
   *
   * + Internal system threads should have no signals enabled. They
   *   have no business executing user signal handlers -- especially IDLE.
   * + The initial signal mask for other threads needs to follow the
   *   implication of a pure C99 environment which only has the methods
   *   raise() and signal(). This implies that all signals are unmasked
   *   until the thread explicitly uses a POSIX methods to block some.
   *   This applies to both Classic tasks and POSIX threads created
   *   as initalization tasks/threads (e.g. before the system is up).
   * + After the initial threads are created, the signal mask should
   *   be inherited from the creator.
   *
   * NOTE: The default signal mask does not matter for any application
   *       that does not use POSIX signals.
   */
  if ( _Objects_Get_API(created->Object.id) == OBJECTS_INTERNAL_API ) {
      /*
       * Ensure internal (especially IDLE) is handled first.
       *
       * Block signals for all internal threads -- especially IDLE.
       */
      api->signals_unblocked = 0;
  } else if ( _Objects_Get_API(executing->Object.id) == OBJECTS_INTERNAL_API ) {
      /*
       * Threads being created while an internal thread is executing
       * should only happen for the initialization threads/tasks.
       *
       * Default state (signals unblocked) for all Initialization tasks
       * and POSIX threads. We should not inherit from IDLE which is
       * what appears to be executing during initialization.
       */
      api->signals_unblocked = SIGNAL_ALL_MASK;
  } else {
    const POSIX_API_Control            *executing_api;
    /*
     * RTEMS is running so follow the POSIX rules to inherit the signal mask.
     */ 
    executing_api = executing->API_Extensions[ THREAD_API_POSIX ];
    api->signals_unblocked = executing_api->signals_unblocked;
  }
#endif
  return true;
}

static void _POSIX_Threads_Terminate_extension( Thread_Control *executing )
{
  POSIX_API_Control *api;
  ISR_lock_Context   lock_context;

  api = executing->API_Extensions[ THREAD_API_POSIX ];

  _Thread_State_acquire( executing, &lock_context );
  _Watchdog_Per_CPU_remove_ticks( &api->Sporadic.Timer );
  _Thread_State_release( executing, &lock_context );
}
#endif

static void _POSIX_Threads_Exitted_extension(
  Thread_Control *executing
)
{
  /*
   *  If the executing thread was not created with the POSIX API, then this
   *  API do not get to define its exit behavior.
   */
  if ( _Objects_Get_API( executing->Object.id ) == OBJECTS_POSIX_API )
    pthread_exit( executing->Wait.return_argument );
}

static User_extensions_Control _POSIX_Threads_User_extensions = {
  .Callouts = {
#if defined(RTEMS_POSIX_API)
    .thread_create    = _POSIX_Threads_Create_extension,
    .thread_terminate = _POSIX_Threads_Terminate_extension,
#endif
    .thread_exitted   = _POSIX_Threads_Exitted_extension
  }
};

static void _POSIX_Threads_Manager_initialization( void )
{
  _Thread_Initialize_information( &_POSIX_Threads_Information );
  _User_extensions_Add_API_set( &_POSIX_Threads_User_extensions );
}

RTEMS_SYSINIT_ITEM(
  _POSIX_Threads_Manager_initialization,
  RTEMS_SYSINIT_POSIX_THREADS,
  RTEMS_SYSINIT_ORDER_MIDDLE
);
