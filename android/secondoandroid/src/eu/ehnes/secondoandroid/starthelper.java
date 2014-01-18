package eu.ehnes.secondoandroid;


/**
 * This class loads the c++ lib and holds the header definitions.
 * 
 * @author juergen
 * @version 1.0
 */
public class starthelper {
	
	//public native int startsecondo(String pfad);
	public native boolean initialize(String configPath);
	public native void shutdown();
	public native Object query(String sAbfrage);
	public native String errorMessage();
	
	static {
		System.loadLibrary("app");
	}
}
