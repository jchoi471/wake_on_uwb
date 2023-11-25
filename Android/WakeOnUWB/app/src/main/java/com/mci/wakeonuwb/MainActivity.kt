package com.mci.wakeonuwb

import android.annotation.SuppressLint
import android.content.Context
import android.net.wifi.WifiManager
import android.os.Bundle
import android.text.method.ScrollingMovementMethod
import android.util.Log
import android.view.View
import android.webkit.WebChromeClient
import android.webkit.WebView
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import android.webkit.WebSettings


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

        val connectButton: Button = findViewById(R.id.connect)

        connectButton.setOnClickListener{
            onConnectButtonClick(it)
        }

        val disconnectButton: Button = findViewById(R.id.disconnect)

        disconnectButton.setOnClickListener {
            onDisconnectButtonClick(it)
        }

        videoDisplay()

    }

    fun onConnectButtonClick(view: View) {
        uwbtransport!!.usbStart()
        showToast("Connect button clicked!")
    }

    fun onDisconnectButtonClick(view: View) {
        val wifi = applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager?
        if (wifi != null && wifi.isWifiEnabled) {
            wifi.setWifiEnabled(false)
        }
        uwbtransport!!.usbStop()
        showToast("Disconnect button clicked!")
    }

    private fun showToast(message: String) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
    }

    override fun onStart() {
        super.onStart()
//        uwbtransport!!.usbStart()
    }

    override fun onDestroy() {
        super.onDestroy()
        uwbtransport!!.usbStop()
    }

    fun display(message: String?) {
        runOnUiThread {
            if (textView != null) {
                Log.d("DisplayFunction", "Message: $message")
                if (!message.isNullOrBlank()) {
                    textView.append(
                        """
                $message
                """.trimIndent()
                    )
                }
            }
        }
    }

    override fun videoDisplay() {
//        try {


        val webView: WebView = findViewById(R.id.webView)
        webView.settings.mediaPlaybackRequiresUserGesture = false

        val videoId = "u_BcMXgws6Y"

//        val video: String = "<iframe width=\"100%\" height=\"100%\" src=\"https://www.youtube.com/embed/u_BcMXgws6Y\" " +
//                            "title=\"YouTube video player\" frameborder=\"0\" allow=\"accelerometer; autoplay; clipboard-write; " +
//                            "encrypted-media; gyroscope; picture-in-picture; web-share\" allowfullscreen></iframe>"

        val video: String = """
        <html>
        <body>
            <iframe width="100%" height="100%" src="https://www.youtube.com/embed/u_BcMXgws6Y?si=hU2ERTI95koxUeVR" 
                title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" allowfullscreen>
            </iframe>
            <script>
                function playVideo() {
                    document.querySelector('iframe').contentWindow.postMessage('{"event":"command","func":"playVideo","args":""}', '*');
                }
            </script>
        </body>
        </html>
    """

        webView.loadData(video, "text/html", "utf-8")
        webView.loadUrl("javascript:playVideo()")
        webView.settings.javaScriptEnabled = true
        webView.settings.pluginState = WebSettings.PluginState.ON
        webView.webChromeClient = WebChromeClient()
//        }
//        catch (e: Exception){
//            e.printStackTrace()
//        }
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
//package com.mci.wakeonuwb
//
//import android.annotation.SuppressLint
//import android.content.Context
//import android.net.wifi.WifiManager
//import android.os.Bundle
//import android.text.method.ScrollingMovementMethod
//import android.util.Log
//import android.view.View
//import android.webkit.WebChromeClient
//import android.webkit.WebView
//import android.widget.Button
//import android.widget.TextView
//import android.widget.Toast
//import androidx.appcompat.app.AppCompatActivity
//
//
//class MainActivity : AppCompatActivity(), UWBtransportListener {
//
//    private var arduino: Arduino? = null
//    private var uwbtransport: UWBTransport? = null
//    private lateinit var textView: TextView
//    var context: Context? = null
//
//    @SuppressLint("ServiceCast")
//    override fun onCreate(savedInstanceState: Bundle?) {
//        super.onCreate(savedInstanceState)
//        setContentView(R.layout.activity_main)
//        context = this
//        textView = findViewById(R.id.textView)
////        textView.movementMethod = ScrollingMovementMethod()
//
//        arduino = Arduino(this)
//        uwbtransport = UWBTransport(this, arduino!!)
//        uwbtransport!!.setUwbtransportlistener(this)
//
//        val connectButton: Button = findViewById(R.id.connect)
//
//        connectButton.setOnClickListener{
//            onConnectButtonClick(it)
//        }
//
//        val disconnectButton: Button = findViewById(R.id.disconnect)
//
//        disconnectButton.setOnClickListener {
//            onDisconnectButtonClick(it)
//        }
//
//        videoDisplay()
//
//    }
//
//    fun onConnectButtonClick(view: View) {
//        uwbtransport!!.usbStart()
//        showToast("Connect button clicked!")
//    }
//
//    fun onDisconnectButtonClick(view: View) {
//        val wifi = applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager?
//        if (wifi != null && wifi.isWifiEnabled) {
//            wifi.setWifiEnabled(false)
//        }
//        uwbtransport!!.usbStop()
//        showToast("Disconnect button clicked!")
//    }
//
//    private fun showToast(message: String) {
//        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
//    }
//
//    override fun onStart() {
//        super.onStart()
////        uwbtransport!!.usbStart()
//    }
//
//    override fun onDestroy() {
//        super.onDestroy()
//        uwbtransport!!.usbStop()
//    }
//
//    fun display(message: String?) {
//        runOnUiThread {
//            textView!!.text = message
////            if (textView != null) {
////                Log.d("DisplayFunction", "Message: $message")
////                if (!message.isNullOrBlank()) {
////                    textView.append(
////                        """
////                $message
////                """.trimIndent()
////                    )
////                }
////            }
//        }
//    }
//
//    override fun videoDisplay() {
//        val webView: WebView = findViewById(R.id.webView)
//        val video: String = "<iframe width=\"100%\" height=\"100%\" src=\"https://www.youtube.com/embed/u_BcMXgws6Y?si=hU2ERTI95koxUeVR\" title=\"YouTube video player\" frameborder=\"0\" allow=\"accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share\" allowfullscreen></iframe>"
//        webView.loadData(video, "text/html", "utf-8")
//        webView.settings.javaScriptEnabled = true
//        webView.webChromeClient = WebChromeClient()
//    }
//
//    override fun onUwbTXSuccess() {
//        TODO("Not yet implemented")
//    }
//
//    override fun onUwbTXFail(tx_err_msg: String?) {
//        TODO("Not yet implemented")
//    }
//
//    override fun onUwbDebugging(msg: String?) {
//        display(msg)
//    }
//}
