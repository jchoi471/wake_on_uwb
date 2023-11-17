package com.example.myapplication

import android.app.PendingIntent
import android.content.Intent
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbManager
import android.net.ConnectivityManager
import android.os.Bundle
import android.text.method.ScrollingMovementMethod
import android.util.Log
import android.widget.Toast
import androidx.activity.ComponentActivity
import com.example.myapplication.databinding.ActivityMainBinding
import com.felhr.usbserial.UsbSerialDevice

class MainActivity : ComponentActivity(), UWBtransportListener {

    val ACTION_USB_PERMISSION = "permission"
    lateinit var usbManager:UsbManager
    var device:UsbDevice? = null
    var serial: UsbSerialDevice? = null
    var connection: UsbDeviceConnection? = null

    // ref: https://developer.android.com/topic/libraries/view-binding/migration
    private lateinit var binding: ActivityMainBinding

    private var arduino: Arduino? = null

    private var uwbtransport: UWBTransport? = null

    override fun onStart() {
        super.onStart()
        uwbtransport!!.usbStart()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.textView.movementMethod = ScrollingMovementMethod()

//        usbManager = getSystemService(Context.USB_SERVICE) as UsbManager
//
//        val filter = IntentFilter()
//        filter.addAction(ACTION_USB_PERMISSION)
//        filter.addAction(UsbManager.ACTION_USB_ACCESSORY_ATTACHED)
//        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
//        registerReceiver(broadcastReceiver, filter)
//
//        binding.on.setOnClickListener{ sendData("o") }
////        binding.off.setOnClickListener{ sendData("x")}
//        binding.disconnect.setOnClickListener{ disconnect() }
//        binding.connect.setOnClickListener{ startUsbConnecting() }

        arduino = Arduino(this)

        uwbtransport = UWBTransport(this, arduino!!)
        uwbtransport!!.setUwbtransportlistener(this)


        // check if the wifi is turned on
//        if (isConnectedToWifi()) {
//            // Display a pop-up message using Toast
//            showToast("Wi-Fi connected successfully!");
//        }


    }

    override fun onDestroy() {
        super.onDestroy()
        uwbtransport!!.usbStop()
    }

    private fun startUsbConnecting() {
        val usbDevices: HashMap<String, UsbDevice>? = usbManager.deviceList
        if (!usbDevices?.isEmpty()!!) {
            var keep = true
            usbDevices.forEach{ entry ->
                device = entry.value
                val deviceVendorId: Int? = device?.vendorId
                Log.i("Serial", "vendorId: " + deviceVendorId)
                if (deviceVendorId == 9114) {
                    val intent: PendingIntent = PendingIntent.getBroadcast(this, 0, Intent(ACTION_USB_PERMISSION),
                        PendingIntent.FLAG_IMMUTABLE)
                    usbManager.requestPermission(device, intent)
                    keep = false
                    Log.i("Serial", "connection successful")
                } else {
                    connection = null
                    device = null
                    Log.i("Serial", "unable to connect")
                }
                if (!keep) {
                    return
                }
            }
        } else {
            Log.i("Serial", "no usb device connected")
        }
    }

    // send data to usb serial
    private fun sendData(input: String) {
        serial?.write(input.toByteArray())
        Log.i("Serial", "sending data: " + input.toByteArray())
    }

    private fun disconnect() {
        serial?.close()
    }



    private fun isConnectedToWifi(): Boolean {
        val connectivityManager = getSystemService(CONNECTIVITY_SERVICE) as ConnectivityManager
        val wifiInfo = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI)
        return wifiInfo != null && wifiInfo.isConnected
    }

    private fun showToast(message: String) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
    }


    override fun onUwbTXSuccess() {
        TODO("Not yet implemented")
    }

    override fun onUwbTXFail(tx_err_msg: String?) {
        TODO("Not yet implemented")
    }

    override fun onUwbDebugging(msg: String?) {
        display(msg)
    }

    fun display(message: String?) {
        runOnUiThread {
            binding.textView.append(
                """
            $message
            """.trimIndent()
            )
        }
    }


}
