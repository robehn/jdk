/*
 * Copyright (c) 2003, 2019, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2020, 2021, Huawei Technologies Co., Ltd. All rights reserved.
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

#ifndef OS_CPU_LINUX_RISCV_ORDERACCESS_LINUX_RISCV_HPP
#define OS_CPU_LINUX_RISCV_ORDERACCESS_LINUX_RISCV_HPP

// Included in orderAccess.hpp header file.

#include "runtime/vm_version.hpp"

// Implementation of class OrderAccess.

inline void OrderAccess::loadload()   { acquire(); }
inline void OrderAccess::storestore() { release(); }
inline void OrderAccess::loadstore()  { acquire(); }
inline void OrderAccess::storeload()  { fence(); }

#define FULL_MEM_BARRIER  __atomic_thread_fence(__ATOMIC_SEQ_CST);
#define READ_MEM_BARRIER  __atomic_thread_fence(__ATOMIC_ACQUIRE);
#define WRITE_MEM_BARRIER __atomic_thread_fence(__ATOMIC_RELEASE);

inline void OrderAccess::acquire() {
  READ_MEM_BARRIER;
}

inline void OrderAccess::release() {
  WRITE_MEM_BARRIER;
}

inline void OrderAccess::fence() {
  FULL_MEM_BARRIER;
}

inline void OrderAccess::cross_modify_fence_impl() {
  // From 3 “Zifencei” Instruction-Fetch Fence, Version 2.0
  // "RISC-V does not guarantee that stores to instruction memory will be made
  // visible to instruction fetches on a RISC-V hart until that hart executes a
  // FENCE.I instruction. A FENCE.I instruction ensures that a subsequent
  // instruction fetch on a RISC-V hart will see any previous data stores
  // already visible to the same RISC-V hart. FENCE.I does not ensure that other
  // RISC-V harts’ instruction fetches will observe the local hart’s stores in a
  // multiprocessor system."
  //
  // If the I cache is updated and if a context switch to a new hart guarantees
  // no stale I cache:
  // The current hart can still execute instructions out-of-order, which means
  // e.g. an immediate to materialize an address may be executed before the
  // 'barrier'. When executing the barrier it may be disarmed and the stale
  // instruction can retire.
  //
  // Note that this also dependent on ICache flush implementation,
  // e.g. using full system IPI before releasing the 'barrier' this is not
  // strictly needed. That implementation is somewhat opaque to us, thus we
  // always emit proper fence.
  if (VM_Version::supports_fencei_barrier()) {
    // It's been requested to use fence.i through a VDSO call instead.
    // This way the kernel could know that no stale I cache is allowed
    // when context switching. There is no such vdso yet.
    __asm__ volatile("fence.i" : : : "memory");
  }
}

#endif // OS_CPU_LINUX_RISCV_ORDERACCESS_LINUX_RISCV_HPP
