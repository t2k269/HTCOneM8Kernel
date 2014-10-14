/*
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

package android.view;

import com.android.internal.view.IInputContext;
import com.android.internal.view.IInputMethodClient;

import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.IOnKeyguardExitResult;
import android.view.KeyEvent;
import android.view.MotionEvent;

/**
 * System private interface to the window manager.
 *
 * {@hide}
 */
interface IWindowManager
{
    /**
     * ===== NOTICE =====
     * The first three methods must remain the first three methods. Scripts
     * and tools rely on their transaction number to work properly.
     */
    // This is used for debugging
    boolean startViewServer(int port);   // Transaction #1
    boolean stopViewServer();            // Transaction #2
    boolean isViewServerRunning();       // Transaction #3

    // these require DISABLE_KEYGUARD permission
    void disableKeyguard(IBinder token, String tag);
    void reenableKeyguard(IBinder token);
    void exitKeyguardSecurely(IOnKeyguardExitResult callback);
    boolean isKeyguardLocked();
    boolean isKeyguardSecure();
    boolean inKeyguardRestrictedInputMode();
    void dismissKeyguard();
}