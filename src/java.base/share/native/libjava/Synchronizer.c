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

#include "jni.h"
#include "jvm.h"

#include "java_lang_Synchronizer.h"

#define ARRAY_LENGTH(a) (sizeof(a)/sizeof(a[0]))

static JNINativeMethod methods[] = {
    {"nlock",            "(Ljava/lang/Object;)V", (void *)&JVM_BJL_lock},
    {"nunlock",          "(Ljava/lang/Object;)V", (void *)&JVM_BJL_unlock},
    {"nwait",            "(Ljava/lang/Object;)V", (void *)&JVM_BJL_wait},
    {"nnotify",          "(Ljava/lang/Object;)V", (void *)&JVM_BJL_notify},
    {"nnotify_all",      "(Ljava/lang/Object;)V", (void *)&JVM_BJL_notify_all},
};

JNIEXPORT void JNICALL Java_java_lang_Synchronizer_registerNatives(JNIEnv *env, jclass cls)
{
    (*env)->RegisterNatives(env, cls, methods, ARRAY_LENGTH(methods));
}

/*
JNIEXPORT void JNICALL Java_java_lang_Synchronizer_nlock(JNIEnv *env, jclass c, jobject x) {
  JVM_BJL_lock(env, x);
}

JNIEXPORT void JNICALL Java_java_lang_Synchronizer_nunlock(JNIEnv *env, jclass c, jobject x) {
  //JVM_BJL_unlock(env, x);
}

JNIEXPORT void JNICALL Java_java_lang_Synchronizer_nwait(JNIEnv *env, jclass c, jobject x) {
  //JVM_BJL_wait(env, x);
}

JNIEXPORT void JNICALL Java_java_lang_Synchronizer_nnotify(JNIEnv *env, jclass c, jobject x) {
  //JVM_BJL_notify(env, x);
}

JNIEXPORT void JNICALL Java_java_lang_Synchronizer_nnotify_all(JNIEnv *env, jclass c, jobject x) {
  //JVM_BJL_notify_all(env, x);
}
*/

