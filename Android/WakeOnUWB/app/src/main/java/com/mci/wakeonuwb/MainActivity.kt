package com.mci.wakeonuwb

import android.annotation.SuppressLint
import android.content.Context
import android.net.ConnectivityManager
import android.os.Bundle
import android.text.method.ScrollingMovementMethod
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity(), UWBtransportListener {

    private var arduino: Arduino? = null
    private var uwbtransport: UWBTransport? = null
    private lateinit var textView: TextView
    var context: Context? = null

    @SuppressLint("ServiceCast")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        context = this
        textView = findViewById(R.id.textView)
        textView.movementMethod = ScrollingMovementMethod()

        arduino = Arduino(this)
        uwbtransport = UWBTransport(this, arduino!!)
        uwbtransport!!.setUwbtransportlistener(this)
    }

    override fun onStart() {
        super.onStart()
        uwbtransport!!.usbStart()
    }

    override fun onDestroy() {
        super.onDestroy()
        uwbtransport!!.usbStop()
    }

    fun display(message: String?) {
        runOnUiThread {
            textView.append(
                """
            $message
            """.trimIndent()
            )
        }
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
}
