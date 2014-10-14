package org.t2k269.sp2whelper;

import java.lang.reflect.Method;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.IOnKeyguardExitResult;
import android.view.IWindowManager;
import android.view.Menu;
import android.view.MenuItem;
import android.view.WindowManager;


public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        final Handler handler = new Handler();
        Thread t = new Thread() {
        	@Override
        	public void run() {
		        try {
		        	Thread.sleep(1000);
		        	Log.d(MainActivity.class.getSimpleName(), "Dismiss keyguard");
		        	Class<?> serviceManagerClass = Class.forName("android.os.ServiceManager");
		        	IWindowManager iwm = IWindowManager.Stub.asInterface((IBinder)serviceManagerClass.getMethod("getService", String.class).invoke(null, "window"));
		        	iwm.dismissKeyguard();
		        } catch (Exception ex) {
		        	ex.printStackTrace();
		        }
		        
		        handler.post(new Runnable() {
					@Override
					public void run() {
				        finish();
					}
		        });
        	}
        };
        t.start();
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        if (id == R.id.action_settings) {
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
}
