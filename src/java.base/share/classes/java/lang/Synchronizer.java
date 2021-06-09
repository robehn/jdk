/*
 * Copyright (c) 2021, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
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
package java.lang;

/**
 * The {@code Monitor} class contains several useful class fields
 * and methods. It cannot be instantiated.
 *
 * Among the facilities provided by the {@code System} class
 * are standard input, standard output, and error output streams;
 * access to externally defined properties and environment
 * variables; a means of loading files and libraries; and a utility
 * method for quickly copying a portion of an array.
 *
 * @since   1.0
 */
public final class Synchronizer {
    private native static void nlock(Object o);
    private native static void nunlock(Object o);
    private native static void nwait(Object o);
    private native static void nnotify(Object o);
    private native static void nnotify_all(Object o);

    private Synchronizer() {
    }

    private static native void registerNatives();
    static {
        registerNatives();
    }


    /**
     * Dummy
     * @param o lock o
     * @return stuff
     */
    public static void enter(Object o) {
        nlock(o);
    }
    /**
     * Dummy
     * @param o lock o
     * @return stuff
     */
    public static void waitDo(Object o) {
        nwait(o);
    }
    /**
     * Dummy
     * @param o lock o
     * @return stuff
     */
    public static void exit(Object o) {
        nunlock(o);
    }
    /**
     * Dummy
     * @param o lock o
     * @return stuff
     */
    public static void exit() {
        nunlock(null);
    }

    /**
     * Dummy
     * @param o lock o
     * @return stuff
     */
    public static void notifyDo(Object o) {
        nnotify(o);
    }
    /**
     * Dummy
     * @param o lock o
     * @return stuff
     */
    public static void notifyAllDo(Object o) {
        nnotify_all(o);
    }
}
