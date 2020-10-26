/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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
 */

/**
 * @test
 * @bug 8167108
 * @summary Stress test java.lang.Thread.suspend() at thread exit.
 * @run main/othervm SuspendResume
 * @run main/othervm -Xlog:handshake*=trace,safepoint*=trace -XX:+UnlockDiagnosticVMOptions -XX:GuaranteedSafepointInterval=1 -XX:+HandshakeALot SuspendResume
 */

public class SuspendResume extends Thread {
    private static volatile boolean done = false;
    @Override
    public void run() {
        while (!done) {
            try {
                mysleep(1);
            } catch (Exception e) {}
            System.out.println("Running");
        }
    }

    public static void mysleep(int times) {
        try {
            Thread.sleep(times * 100);
        } catch (Exception e) {}
    }

    public static void main(String[] args) {
        SuspendResume thread = new SuspendResume();
        thread.start();
        thread.suspend();
        mysleep(10);
        thread.resume();
        mysleep(10);
        thread.suspend();
        mysleep(10);
        thread.resume();

        done = true;
        try {
            thread.join();
        } catch (InterruptedException e) {
            throw new Error("Unexpected: " + e);
        }
    }
}
