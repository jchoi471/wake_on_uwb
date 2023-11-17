package com.example.myapplication

import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbManager
import android.util.Log
import com.example.myapplication.ArduinoListener
import com.felhr.usbserial.UsbSerialDevice
import com.felhr.usbserial.UsbSerialInterface

/**
 * Created by Omar on 21/05/2017.
 */

class Arduino : UsbSerialInterface.UsbReadCallback {
    private var context: Context? = null
    private var listener: ArduinoListener? = null
    private var connection: UsbDeviceConnection? = null
    private var serialPort: UsbSerialDevice? = null
    private var usbReceiver: UsbReceiver? = null
    private var usbManager: UsbManager? = null
    private var lastArduinoAttached: UsbDevice? = null
    private var baudRate = 0
    var isOpened = false
        private set
    private var vendorIds: MutableList<Int>? = null
    private var bytesReceived: List<Byte>? = null
    private var delimiter: Byte = 0

    constructor(context: Context, baudRate: Int) {
        init(context, baudRate)
    }

    constructor(context: Context) {
        init(context, DEFAULT_BAUD_RATE)
    }

    private fun init(context: Context, baudRate: Int) {
        this.context = context
        usbReceiver = UsbReceiver()
        usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
        this.baudRate = baudRate
        isOpened = false
        vendorIds = ArrayList()
        (vendorIds as ArrayList<Int>).add(9025)
        (vendorIds as ArrayList<Int>).add(9114)
        bytesReceived = ArrayList()
        delimiter = DEFAULT_DELIMITER
    }

    fun setArduinoListener(listener: ArduinoListener?) {
        this.listener = listener
        val intentFilter = IntentFilter()
        intentFilter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
        intentFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        intentFilter.addAction(ACTION_USB_DEVICE_PERMISSION)
        this.context!!.registerReceiver(usbReceiver, intentFilter)
        lastArduinoAttached = attachedArduino
        if (lastArduinoAttached != null && listener != null) {
            listener.onArduinoAttached(lastArduinoAttached)
        }
    }

    fun unsetArduinoListener() {
        listener = null
    }

    fun open(device: UsbDevice?) {
        val permissionIntent = PendingIntent.getBroadcast(
            context, 0, Intent(
                ACTION_USB_DEVICE_PERMISSION
            ), PendingIntent.FLAG_IMMUTABLE
        )
        val filter = IntentFilter()
        filter.addAction(ACTION_USB_DEVICE_PERMISSION)
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        context!!.registerReceiver(usbReceiver, filter)
        usbManager!!.requestPermission(device, permissionIntent)
    }

    fun reopen() {
        open(lastArduinoAttached)
    }

    fun close() {
        if (serialPort != null) {
            serialPort!!.close()
        }
        if (connection != null) {
            connection!!.close()
        }
        isOpened = false
        context!!.unregisterReceiver(usbReceiver)
    }

    fun send(bytes: ByteArray?) {
        if (serialPort != null) {
            serialPort!!.write(bytes)
        }
    }

    fun setDelimiter(delimiter: Byte) {
        this.delimiter = delimiter
    }

    fun setBaudRate(baudRate: Int) {
        this.baudRate = baudRate
    }

    fun addVendorId(id: Int) {
        vendorIds!!.add(id)
    }

    private inner class UsbReceiver : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val device: UsbDevice?
            if (intent.action != null) {
                when (intent.action) {
                    UsbManager.ACTION_USB_DEVICE_ATTACHED -> {
                        device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE)
                        if (hasId(device!!.vendorId)) {
                            lastArduinoAttached = device
                            listener?.onArduinoAttached(device)
                        }
                    }

                    UsbManager.ACTION_USB_DEVICE_DETACHED -> {
                        device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE)
                        if (hasId(device!!.vendorId)) {
                            listener?.onArduinoDetached()
                        }
                    }

                    ACTION_USB_DEVICE_PERMISSION -> if (intent.getBooleanExtra(
                            UsbManager.EXTRA_PERMISSION_GRANTED,
                            false
                        )
                    ) {
                        device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE)
                        if (hasId(device!!.vendorId)) {
                            connection = usbManager!!.openDevice(device)
                            serialPort = UsbSerialDevice.createUsbSerialDevice(device, connection)
                            if (serialPort != null) {
                                if (serialPort!!.open()) {
                                    serialPort!!.setBaudRate(baudRate)
                                    serialPort!!.setDataBits(UsbSerialInterface.DATA_BITS_8)
                                    serialPort!!.setStopBits(UsbSerialInterface.STOP_BITS_1)
                                    serialPort!!.setParity(UsbSerialInterface.PARITY_NONE)
                                    serialPort!!.setFlowControl(UsbSerialInterface.FLOW_CONTROL_OFF)
                                    serialPort!!.read(this@Arduino)
                                    isOpened = true
                                    listener?.onArduinoOpened()
                                }
                            }
                        }
                    } else listener?.onUsbPermissionDenied()
                }
            }
        }
    }

    private val attachedArduino: UsbDevice?
        private get() {
            val map = usbManager!!.deviceList
            for (device in map.values) {
                if (hasId(device.vendorId)) {
                    return device
                }
            }
            return null
        }

    override fun onReceivedData(bytes: ByteArray) {
        listener?.onArduinoMessage(bytes)
    }

    private fun hasId(id: Int): Boolean {
        Log.i(javaClass.simpleName, "Vendor id : $id")
        for (vendorId in vendorIds!!) {
            if (vendorId == id) {
                return true
            }
        }
        return false
    }

    companion object {
        private const val ACTION_USB_DEVICE_PERMISSION = "me.aflak.arduino.USB_PERMISSION"
        private const val DEFAULT_BAUD_RATE = 9600
        private const val DEFAULT_DELIMITER = '\n'.code.toByte()
    }
}