# SDL registers Java callbacks by name from native code, not Java references.
-keep class org.libsdl.app.** { *; }
-keepclasseswithmembernames,includedescriptorclasses class * { native <methods>; }
-keepclassmembers class org.p4pc.game.P4Activity {
    public void trackHardwareKeyboard(boolean);
    public boolean prepareRestart();
}
