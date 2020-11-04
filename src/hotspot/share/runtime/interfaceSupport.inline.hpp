/*
 * Copyright (c) 1997, 2021, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef SHARE_RUNTIME_INTERFACESUPPORT_INLINE_HPP
#define SHARE_RUNTIME_INTERFACESUPPORT_INLINE_HPP

#include "gc/shared/gc_globals.hpp"
#include "runtime/handles.inline.hpp"
#include "runtime/mutexLocker.hpp"
#include "runtime/orderAccess.hpp"
#include "runtime/os.hpp"
#include "runtime/safepointMechanism.inline.hpp"
#include "runtime/safepointVerifiers.hpp"
#include "runtime/thread.hpp"
#include "runtime/vmOperations.hpp"
#include "utilities/globalDefinitions.hpp"
#include "utilities/macros.hpp"
#include "utilities/preserveException.hpp"

// Wrapper for all entry points to the virtual machine.

// InterfaceSupport provides functionality used by the VM_LEAF_BASE and
// VM_ENTRY_BASE macros. These macros are used to guard entry points into
// the VM and perform checks upon leave of the VM.


class InterfaceSupport: AllStatic {
# ifdef ASSERT
 public:
  static unsigned int _scavenge_alot_counter;
  static unsigned int _fullgc_alot_counter;
  static int _fullgc_alot_invocation;

  // Helper methods used to implement +ScavengeALot and +FullGCALot
  static void check_gc_alot() { if (ScavengeALot || FullGCALot) gc_alot(); }
  static void gc_alot();

  static void walk_stack_from(vframe* start_vf);
  static void walk_stack();

  static void zombieAll();
  static void deoptimizeAll();
  static void verify_stack();
  static void verify_last_frame();
# endif
};



// Basic class for all thread transition classes.
// To  \  From  ||   java    |          native           |           vm             |          blocked          |   new    |
// -------------||-----------|---------------------------|--------------------------|---------------------------|----------|
//              ||           |    safepoint/handshakes   |   safepoint/handshakes   |                           |          |
//    java      ||    XXX    |     suspend/resume        |     suspend/resume       |            XXX            |   XXX    |
//              ||           |       JFR sampling        |      JFR sampling        |                           |          |
//              ||           |     async exceptions      |     async exceptions     |                           |          |
// -------------||-----------|---------------------------|--------------------------|---------------------------|----------|
//              ||           |                           |                          |                           |          |
//    native    ||   None    |           XXX             |          None            |            XXX            |   XXX    |
//              ||           |                           |                          |                           |          |
// -------------||-----------|---------------------------|--------------------------|---------------------------|----------|
//              ||           |                           |                          |                           |          |
//      vm      ||   None    |    safepoint/handshakes   |           XXX            |    safepoint/handshakes   |   None   |
//              ||           |                           |                          |                           |          |
// -------------||-----------|---------------------------|--------------------------|---------------------------|----------|
//    blocked   ||    XXX    |           XXX             |          None            |            XXX            |   XXX    |
// -------------||-----------|---------------------------|--------------------------|---------------------------|----------|

template<JavaThreadState JTS_FROM, JavaThreadState JTS_TO>
class Transition {
};

template<>
class Transition<_thread_in_vm, _thread_in_Java> {
 public:
  static inline void trans(JavaThread *thread, bool async = false) {
    assert(thread->thread_state() == _thread_in_vm, "coming from wrong thread state");
    if (thread->stack_overflow_state()->stack_yellow_reserved_zone_disabled()) {
      thread->stack_overflow_state()->enable_stack_yellow_reserved_zone();
    }
    // Check NoSafepointVerifier
    // This also clears unhandled oops if CheckUnhandledOops is used.
    thread->check_possible_safepoint();

    SafepointMechanism::process_if_requested_with_exit_check(thread, async);
    thread->set_thread_state(_thread_in_Java);
  }
};

template<>
class Transition<_thread_in_native, _thread_in_Java> : public Transition<_thread_in_vm, _thread_in_Java> {};

template<>
class Transition<_thread_blocked, _thread_in_vm> {
 public:
  static inline void trans(JavaThread *thread) {
    // static assert
    assert(thread->thread_state() == _thread_in_native || 
           thread->thread_state() == _thread_blocked   ||
           thread->thread_state() == _thread_new, "Must be");

    // Check NoSafepointVerifier
    // This also clears unhandled oops if CheckUnhandledOops is used.
    thread->check_possible_safepoint();

    thread->set_thread_state_fence(_thread_in_vm);
    SafepointMechanism::process_if_requested(thread);
  }
};

template<>
class Transition<_thread_in_native, _thread_in_vm> : public Transition<_thread_blocked, _thread_in_vm> {};

template<>
class Transition<_thread_new, _thread_in_vm> : public Transition<_thread_blocked, _thread_in_vm> {};

template<JavaThreadState JTS_TO>
class Transition<_thread_in_vm, JTS_TO> {
 public:
  static inline void trans(JavaThread *thread) {
    assert(thread->thread_state() != _thread_in_native && thread->thread_state() != _thread_blocked, "Must be");
    assert(JTS_TO != _thread_in_Java, "Must use to Java");
    assert(thread->is_Compiler_thread() || !thread->owns_locks() || JTS_TO != _thread_in_native, "must release all locks when leaving VM");
    thread->set_thread_state(JTS_TO);
  }
};

template<JavaThreadState JTS_TO>
class Transition<_thread_in_Java, JTS_TO> {
 public:
  static inline void trans(JavaThread *thread) {
    assert(thread->thread_state() != _thread_in_native && thread->thread_state() != _thread_blocked, "Must be");
    assert(thread->is_Compiler_thread() || !thread->owns_locks() || JTS_TO != _thread_in_native, "must release all locks when leaving VM");
    assert(JTS_TO == _thread_in_native || JTS_TO == _thread_in_vm, "Must be");
    thread->frame_anchor()->make_walkable(thread);
    thread->set_thread_state(JTS_TO);
  }
};

template<JavaThreadState JTS_FROM, JavaThreadState JTS_TO, bool ASYNC = false>
class ThreadStateTransition {
 protected:
  JavaThread* _thread;
 public:
  ThreadStateTransition(JavaThread* thread) : _thread(thread) {
    Transition<JTS_FROM, JTS_TO>::trans(_thread);
  }
  ~ThreadStateTransition() {
    if (ASYNC) {
      assert(JTS_TO == _thread_in_vm && JTS_FROM == _thread_in_Java, "Must be");
      Transition<_thread_in_vm, _thread_in_Java>::trans(_thread, true);
    } else {
      Transition<JTS_TO, JTS_FROM>::trans(_thread);
    }
  }
};

typedef           ThreadStateTransition<_thread_in_Java,   _thread_in_vm,  true> ThreadInVMfromJava;
typedef           ThreadStateTransition<_thread_in_Java,   _thread_in_vm   >     ThreadInVMfromJavaNoAsyncException;
typedef           ThreadStateTransition<_thread_in_native, _thread_in_vm   >     ThreadInVMfromNative;
typedef           ThreadStateTransition<_thread_in_vm,     _thread_blocked >     ThreadBlockInVM;

//<<<<<<< HEAD FIXME
//class ThreadInVMfromNative : public ThreadStateTransition {
//  ResetNoHandleMark __rnhm;
//=======

class ThreadToNativeFromVM : public ThreadStateTransition<_thread_in_vm, _thread_in_native> {
 private:
  HandleMark _hm;
 public:
  ThreadToNativeFromVM(JavaThread* thread) : ThreadStateTransition<_thread_in_vm, _thread_in_native, false>::ThreadStateTransition(thread), _hm(thread) {}
  ~ThreadToNativeFromVM() {}
};

class ThreadInVMfromUnknown {
  JavaThread*     _thread;
  JavaThreadState _state;
 public:
  ThreadInVMfromUnknown() : _thread(NULL) {
    Thread* thread = Thread::current();
    if (thread == NULL || !thread->is_Java_thread()) {
      return;
    }
    JavaThread* jt = thread->as_Java_thread(); 
    _state = jt->thread_state();
    if (_state == _thread_in_vm) {
      return;
    }
    _thread = jt;
    if (_state == _thread_in_native) {
      Transition<_thread_in_native, _thread_in_vm>::trans(_thread);
    } else {
      assert(_state == _thread_in_Java, "Must be");
      Transition<_thread_in_Java, _thread_in_vm>::trans(_thread);
    }
  }
  ~ThreadInVMfromUnknown()  {
    if (_thread) {
      if (_state == _thread_in_native) {
        Transition<_thread_in_vm, _thread_in_native>::trans(_thread);
      } else {
        assert(_state == _thread_in_Java, "Must be");
        Transition<_thread_in_vm, _thread_in_Java>::trans(_thread);
      }
    }
  }
};

class JvmtiThreadEventTransition {
  HandleMark      _hm;
  JavaThread*     _thread;
  JavaThreadState _state;
 public:
  JvmtiThreadEventTransition(JavaThread* thread) : _hm(thread) { 
    if (thread == NULL || !thread->is_Java_thread()) {
      return;
    }
    JavaThread* jt = thread->as_Java_thread(); 
    _state = jt->thread_state();
    if (_state == _thread_in_native) {
      return;
    }
    _thread = jt;
    if (_state == _thread_in_vm) {
      Transition<_thread_in_vm, _thread_in_native>::trans(_thread);
    } else {
      assert(_state == _thread_in_Java , "Must be");
      Transition<_thread_in_Java, _thread_in_native>::trans(_thread);
    }
  }
  ~JvmtiThreadEventTransition()  {
    if (_thread) {
      if (_state == _thread_in_vm) {
        Transition<_thread_in_native, _thread_in_vm>::trans(_thread);
      } else {
        assert(_state == _thread_in_Java, "Must be");
        Transition<_thread_in_native, _thread_in_Java>::trans(_thread);
      }
    }
  }
};

// Unlike ThreadBlockInVM, this class is designed to avoid certain deadlock scenarios while making
// transitions inside class Mutex in cases where we need to block for a safepoint or handshake. It
// receives an extra argument compared to ThreadBlockInVM, the address of a pointer to the mutex we
// are trying to acquire. This will be used to access and release the mutex if needed to avoid
// said deadlocks.
// It works like ThreadBlockInVM but differs from it in two ways:
// - When transitioning in (constructor), it checks for safepoints without blocking, i.e., calls
//   back if needed to allow a pending safepoint to continue but does not block in it.
// - When transitioning back (destructor), if there is a pending safepoint or handshake it releases
//   the mutex that is only partially acquired.
class ThreadBlockInVMWithDeadlockCheck {
 private:
  JavaThread* _thread;
  Mutex** _in_flight_mutex_addr;

  void release_mutex() {
    assert(_in_flight_mutex_addr != NULL, "_in_flight_mutex_addr should have been set on constructor");
    Mutex* in_flight_mutex = *_in_flight_mutex_addr;
    if (in_flight_mutex != NULL) {
      in_flight_mutex->release_for_safepoint();
      *_in_flight_mutex_addr = NULL;
    }
  }
 public:
  ThreadBlockInVMWithDeadlockCheck(JavaThread* thread, Mutex** in_flight_mutex_addr)
  : _thread(thread), _in_flight_mutex_addr(in_flight_mutex_addr) {
    // All unsafe states are treated the same by the VMThread
    // so we can skip the _thread_in_vm_trans state here. Since
    // we don't read poll, it's enough to order the stores.
    OrderAccess::storestore();

    thread->set_thread_state(_thread_blocked);
  }
  ~ThreadBlockInVMWithDeadlockCheck() {
    // Change to transition state and ensure it is seen by the VM thread.
    _thread->set_thread_state_fence(_thread_in_vm);

    if (SafepointMechanism::should_process(_thread)) {
      release_mutex();
      SafepointMechanism::process_if_requested(_thread);
    }
  }
};



// Debug class instantiated in JRT_ENTRY macro.
// Can be used to verify properties on enter/exit of the VM.

#ifdef ASSERT
class VMEntryWrapper {
 public:
  VMEntryWrapper();
  ~VMEntryWrapper();
};


class VMNativeEntryWrapper {
 public:
  VMNativeEntryWrapper();
  ~VMNativeEntryWrapper();
};

#endif // ASSERT

// LEAF routines do not lock, GC or throw exceptions

#define VM_LEAF_BASE(result_type, header)                            \
  debug_only(NoHandleMark __hm;)                                     \
  os::verify_stack_alignment();                                      \
  /* begin of body */

#define VM_ENTRY_BASE_FROM_LEAF(result_type, header, thread)         \
  debug_only(ResetNoHandleMark __rnhm;)                              \
  HandleMarkCleaner __hm(thread);                                    \
  Thread* THREAD = thread;                                           \
  os::verify_stack_alignment();                                      \
  /* begin of body */


// ENTRY routines may lock, GC and throw exceptions

#define VM_ENTRY_BASE(result_type, header, thread)                   \
  HandleMarkCleaner __hm(thread);                                    \
  Thread* THREAD = thread;                                           \
  os::verify_stack_alignment();                                      \
  /* begin of body */


#define JRT_ENTRY(result_type, header)                               \
  result_type header {                                               \
    ThreadInVMfromJava __tiv(thread);                                \
    VM_ENTRY_BASE(result_type, header, thread)                       \
    debug_only(VMEntryWrapper __vew;)

// JRT_LEAF currently can be called from either _thread_in_Java or
// _thread_in_native mode.
//
// JRT_LEAF rules:
// A JRT_LEAF method may not interfere with safepointing by
//   1) acquiring or blocking on a Mutex or JavaLock - checked
//   2) allocating heap memory - checked
//   3) executing a VM operation - checked
//   4) executing a system call (including malloc) that could block or grab a lock
//   5) invoking GC
//   6) reaching a safepoint
//   7) running too long
// Nor may any method it calls.

#define JRT_LEAF(result_type, header)                                \
  result_type header {                                               \
  VM_LEAF_BASE(result_type, header)                                  \
  debug_only(NoSafepointVerifier __nsv;)


#define JRT_ENTRY_NO_ASYNC(result_type, header)                      \
  result_type header {                                               \
    ThreadInVMfromJavaNoAsyncException __tiv(thread);                \
    VM_ENTRY_BASE(result_type, header, thread)                       \
    debug_only(VMEntryWrapper __vew;)

// Same as JRT Entry but allows for return value after the safepoint
// to get back into Java from the VM
#define JRT_BLOCK_ENTRY(result_type, header)                         \
  result_type header {                                               \
    HandleMarkCleaner __hm(thread);

#define JRT_BLOCK                                                    \
    {                                                                \
    ThreadInVMfromJava __tiv(thread);                                \
    Thread* THREAD = thread;                                         \
    debug_only(VMEntryWrapper __vew;)

#define JRT_BLOCK_NO_ASYNC                                           \
    {                                                                \
    ThreadInVMfromJavaNoAsyncException __tiv(thread);                \
    Thread* THREAD = thread;                                         \
    debug_only(VMEntryWrapper __vew;)

#define JRT_BLOCK_END }

#define JRT_END }

// Definitions for JNI

#define JNI_ENTRY(result_type, header)                               \
    JNI_ENTRY_NO_PRESERVE(result_type, header)                       \
    WeakPreserveExceptionMark __wem(thread);

#define JNI_ENTRY_NO_PRESERVE(result_type, header)                   \
extern "C" {                                                         \
  result_type JNICALL header {                                       \
    JavaThread* thread=JavaThread::thread_from_jni_environment(env); \
    assert( !VerifyJNIEnvThread || (thread == Thread::current()), "JNIEnv is only valid in same thread"); \
    ThreadInVMfromNative __tiv(thread);                              \
    debug_only(VMNativeEntryWrapper __vew;)                          \
    VM_ENTRY_BASE(result_type, header, thread)


#define JNI_LEAF(result_type, header)                                \
extern "C" {                                                         \
  result_type JNICALL header {                                       \
    JavaThread* thread=JavaThread::thread_from_jni_environment(env); \
    assert( !VerifyJNIEnvThread || (thread == Thread::current()), "JNIEnv is only valid in same thread"); \
    VM_LEAF_BASE(result_type, header)


// Close the routine and the extern "C"
#define JNI_END } }



// Definitions for JVM

#define JVM_ENTRY(result_type, header)                               \
extern "C" {                                                         \
  result_type JNICALL header {                                       \
    JavaThread* thread=JavaThread::thread_from_jni_environment(env); \
    ThreadInVMfromNative __tiv(thread);                              \
    debug_only(VMNativeEntryWrapper __vew;)                          \
    VM_ENTRY_BASE(result_type, header, thread)


#define JVM_ENTRY_NO_ENV(result_type, header)                        \
extern "C" {                                                         \
  result_type JNICALL header {                                       \
    JavaThread* thread = JavaThread::current();                      \
    ThreadInVMfromNative __tiv(thread);                              \
    debug_only(VMNativeEntryWrapper __vew;)                          \
    VM_ENTRY_BASE(result_type, header, thread)


#define JVM_LEAF(result_type, header)                                \
extern "C" {                                                         \
  result_type JNICALL header {                                       \
    VM_Exit::block_if_vm_exited();                                   \
    VM_LEAF_BASE(result_type, header)


#define JVM_ENTRY_FROM_LEAF(env, result_type, header)                \
  { {                                                                \
    JavaThread* thread=JavaThread::thread_from_jni_environment(env); \
    ThreadInVMfromNative __tiv(thread);                              \
    debug_only(VMNativeEntryWrapper __vew;)                          \
    VM_ENTRY_BASE_FROM_LEAF(result_type, header, thread)


#define JVM_END } }

#endif // SHARE_RUNTIME_INTERFACESUPPORT_INLINE_HPP
