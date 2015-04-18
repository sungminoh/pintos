/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
      ////////////////////////////////////////
      // prj(donation) - sungmin oh - start //
      // if current thread want to use shared resources, it needs semaphore.
      // to get semaphore, it have to wait in sema->waiters.
      // we are going to maintain this list ordered by priority
      // we can achieve this condition because there are no more insert isntruction for sema->waiters.
      list_insert_ordered(&sema->waiters, &thread_current()->elem, (list_less_func*)&higher_priority, NULL);
      
      /* original code. no need anymore.
       *
      list_push_back (&sema->waiters, &thread_current ()->elem);
      *
      */
      // prj(donation) - sungmin oh - end //
      //////////////////////////////////////
      thread_block ();
   }
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  
  // prj1(donation - sungmin oh - start) //
  struct thread* highest_thread = NULL;
  // prj1(donation) - sungmin oh - end) //
  
  if (!list_empty (&sema->waiters)){ 
    // prj1(donation) - sungmin oh - start //
    list_sort(&sema->waiters, (list_less_func*)&higher_priority, NULL);
    
    highest_thread = list_entry(list_front(&sema->waiters), struct thread, elem);
    
    /* original code
     */
    thread_unblock (list_entry (list_pop_front (&sema->waiters),
                                struct thread, elem));
    /*
    */
    // prj1(donation) - sungmin oh - end //

  }
  sema->value++;

  // prj1(donation) - sungmin oh - start //
  //intr_set_level (old_level);

  if(!intr_context() && (highest_thread != NULL) && (highest_thread->priority > thread_get_priority())){
    thread_yield();
  }
  /*
  if(!intr_context()){
    priority_check();
  }
  */
  // prj1(donation - sungmin oh - end //

  intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */


/////////////////////////////////////////
// prj1(donation) - sungmin oh - start //
void
priority_donation(struct lock* target_lock){

  struct thread* cur = thread_current();
  // when loack is not null and there is holder already
  if((target_lock != NULL) && (target_lock->holder != NULL)){
    struct thread* lock_holder = target_lock->holder;
    // when lock_hodler's priority is less than current thread which needs same lock, currrent thread donate priority to lock_holder.
    if(lock_holder->priority < cur->priority){
      // if this is first donation for lock_holder, memorize it's original priority
      if(!lock_holder->donated){
        lock_holder->original_priority = lock_holder->priority;//??
        lock_holder->donated = true;
      }

      lock_holder->priority = cur->priority;  // donation
      // if lock_holder is locked because it needs another lock which is already taken, priority_donation have to be executed recursively. else the value of lock_holder->locked is null, recursive call will not do anything.
      priority_donation(lock_holder->locked);
    } 
  }
}

// prj1(donation) - sungmin oh - end //
///////////////////////////////////////




void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  /////////////////////////////////////////
  // prj1(donation) - sungmin oh - start //
  struct thread* cur = thread_current();
  // donate priority to holder of this lock.
  priority_donation(lock);

  // try to get lock
  cur->locked = lock;  
  sema_down(&lock->semaphore);

  // if current thread get lock
  cur->locked = NULL;
  lock->holder = cur;
  list_push_front(&cur->lock_list, &lock->elem);

  /* original code
   *
  sema_down (&lock->semaphore);
  lock->holder = thread_current ();
  *
  */

  // prj1(donation) - sungmin oh - end //
  ///////////////////////////////////////


}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/////////////////////////////////////////
// prj1(donation) - sungmin oh - start //
bool
cmp_sema(struct semaphore* A, struct semaphore* B){
  struct list* a_waiters = &A->waiters;
  struct list* b_waiters = &B->waiters;

  if(list_empty(b_waiters)){
    return false;
  }else if(list_empty(a_waiters)){
    return true;
  }else{
    list_sort(a_waiters, (list_less_func*)&higher_priority, NULL);
    list_sort(b_waiters, (list_less_func*)&higher_priority, NULL);
    struct thread* a_highest_thread = list_entry(list_front(a_waiters), struct thread, elem);
    struct thread* b_highest_thread = list_entry(list_front(b_waiters), struct thread, elem);

    return a_highest_thread->priority < b_highest_thread->priority;
  }
}

bool
higher_lock(const struct list_elem* A, const struct list_elem* B, void* aux_unused){
  struct lock* a_lock = list_entry(A, struct lock, elem);
  struct lock* b_lock = list_entry(B, struct lock, elem);
  struct semaphore* a_sema = &a_lock->semaphore;
  struct semaphore* b_sema = &b_lock->semaphore;

  return cmp_sema(a_sema,b_sema);
}



void
donation_rollback(struct lock* target_lock){
  // don't know why this code is nessessary. but priority_fifo is failed without this code   - g
  //enum intr_level old_level = intr_disable();
  list_remove(&target_lock->elem);
  //enum intr_level old_level = intr_disable();
  // remove lock from lock_list of its holder
  struct thread* holder = thread_current();
  if(list_empty(&(target_lock->semaphore).waiters)){
    return;
  } 
  //??
  if(!holder->donated){
    return;
  }


  if(list_empty(&holder->lock_list)){
    holder->priority = holder->original_priority;
    holder->donated = false;
  }else{
    struct lock* highest_lock = list_entry(list_max(&holder->lock_list, (list_less_func*)&higher_lock, NULL), struct lock, elem);
    struct list* lock_waiters = &(highest_lock->semaphore).waiters;

    if(list_empty(lock_waiters)){
      holder->priority = holder->original_priority;
      holder->donated = false;
    }else{
      struct thread* highest_thread = list_entry(list_front(lock_waiters), struct thread, elem);
      int next_priority = highest_thread->priority;
      if(holder->original_priority >= next_priority){
        holder->priority = holder->original_priority;
        holder->donated = false;
      }else{
        holder->priority = next_priority;
      }
    } 
    //intr_set_level(old_level);
  }
  //intr_set_level(old_level);

 /* 
  struct lock* next_lock = holder->locked;
  if(next_lock != NULL){
    donation_rollback(next_lock, next_lock->holder);//target_lock->holder );   
  }
  */
}

// prj1(donation) - sungmin oh - end //
///////////////////////////////////////

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  /////////////////////////////////////////
  // prj1(donation) - sungmin oh - start //
  // restore priority
//  enum intr_level old_level = intr_disable();
//  list_remove(&lock->elem);
  donation_rollback(lock);
 // intr_set_level(old_level);
  // prj1(donation) - sungmin oh - end //
  ///////////////////////////////////////

  lock->holder = NULL;
  sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}
/* One semaphore in a list. */
struct semaphore_elem 
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
  };

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */

// prj1(cond) - sungmin oh - start //
bool
higher_cond(const struct list_elem* A, const struct list_elem* B, void* aux_unused){
  struct semaphore_elem* a = list_entry(A, struct semaphore_elem, elem);
  struct semaphore_elem* b = list_entry(B, struct semaphore_elem, elem);
  struct list* a_waiters = &(a->semaphore).waiters;
  struct list* b_waiters = &(b->semaphore).waiters;

  if(list_empty(b_waiters)){
    return true;
  }else if(list_empty(a_waiters)){
    return false;
  }else{
    list_sort(a_waiters, (list_less_func*)&higher_priority, NULL);
    list_sort(b_waiters, (list_less_func*)&higher_priority, NULL);
    struct thread* a_highest_thread = list_entry(list_front(a_waiters), struct thread, elem);
    struct thread* b_highest_thread = list_entry(list_front(b_waiters), struct thread, elem);

    return a_highest_thread->priority > b_highest_thread->priority; 
  } 
}
// prj1(cond) - sungmin oh - end //

void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  
  sema_init (&waiter.semaphore, 0);
  // prj1(cond) - sungmin oh - start //
  list_insert_ordered(&cond->waiters, &waiter.elem, (list_less_func*)&higher_cond, NULL);
  // prj1(cond) - sungmin oh - end //
  /* original code
   *
  list_push_back (&cond->waiters, &waiter.elem);
  *
  */
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) 
    // prj1(cond) - sungmin oh - start //
    list_sort(&cond->waiters, (list_less_func*)&higher_cond, NULL);
    // prj1(cond) - sungmin oh - end //
    sema_up (&list_entry (list_pop_front (&cond->waiters),
                          struct semaphore_elem, elem)->semaphore);
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}
