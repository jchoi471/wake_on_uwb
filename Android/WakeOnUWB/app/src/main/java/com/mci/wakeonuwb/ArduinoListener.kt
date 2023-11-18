package com.mci.wakeonuwb

import android.hardware.usb.UsbDevice

interface ArduinoListener {
    fun onArduinoAttached(device: UsbDevice?)
    fun onArduinoDetached()
    fun onArduinoMessage(bytes: ByteArray?)
    fun onArduinoOpened()
    fun onUsbPermissionDenied()
}