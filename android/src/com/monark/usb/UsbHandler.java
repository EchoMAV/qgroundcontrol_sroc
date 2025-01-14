package com.echomav.usb;

import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;

public class UsbHandler {
    // Sample usage:
    // QAndroidJniObject usbHandler = QAndroidJniObject("com/monark/usb/UsbHandler",
    //     "(Landroid/content/Context;)V",
    //     QtAndroid::androidContext().object());
    // usbHandler.callMethod<void>("connect", "(Landroid/hardware/usb/UsbDevice;)V", usbDeviceObject);


    private UsbManager usbManager;
    private UsbSerialPort serialPort;

    public UsbHandler(UsbManager manager) {
        usbManager = manager;
    }

    public void connect(UsbDevice device) {
        serialPort = UsbSerialProber.getDefaultProber().probeDevice(device).getPorts().get(0);
        // Configure the port...
    }
}