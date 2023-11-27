package com.mci.wakeonuwb

import android.content.Context
import android.hardware.usb.UsbDevice
import android.net.wifi.WifiManager
import android.os.Handler
import android.os.Looper
import java.util.Random
class UWBTransport (private val context: Context, var arduino: Arduino) : ArduinoListener {

    private var uwbtransportlistener: UWBtransportListener? = null

    enum class State {
        IDLE, XMIT, RECV
    }

    var state: State
    var MAX_TRANSPORT_LEN = 999 - 12
    var MAX_TRANSPORT_PAYLOAD_LEN = MAX_TRANSPORT_LEN - TRANSPORT_DATA_OFFSET

    inner class Frame {
        var frame_id = 0
        var total_bytes = 0
        var total_num_seg = 0
        lateinit var data: ByteArray
        var curr_seq = 0
        var curr_byte_idx = 0
        var num_bytes_curr_segment = 0
    }

    var tx_frame: Frame? = null
    var rx_frame: Frame? = null

    init {
        state = State.IDLE
    }

    fun setUwbtransportlistener(uwbtransportlistener: UWBtransportListener?) {
        this.uwbtransportlistener = uwbtransportlistener
    }

    /* USB Communication */
    fun usbStart() {
        arduino.setArduinoListener(this)
    }

    fun usbStop() {
        arduino.unsetArduinoListener()
        arduino.close()
    }

    override fun onArduinoAttached(device: UsbDevice?) {
//        display("Arduino attached!");
        arduino.open(device)
    }

    override fun onArduinoDetached() {
    }

    fun transmitFrame(frame: ByteArray, total_bytes: Int): Boolean {
        return if (state == State.IDLE && total_bytes > 0) {
            uwbtransportlistener?.onUwbDebugging("Transmitting...")
            tx_frame = Frame()
            tx_frame!!.data = frame
            tx_frame!!.total_bytes = total_bytes
            tx_frame!!.total_num_seg =
                Math.ceil((total_bytes.toFloat() / MAX_TRANSPORT_PAYLOAD_LEN.toFloat()).toDouble())
                    .toInt()
            val rand = Random()
            tx_frame!!.frame_id = rand.nextInt(255)
            tx_frame!!.curr_byte_idx = 0
            tx_frame!!.curr_seq = 0
            if (tx_frame!!.total_num_seg > 1) //If need to send in multiple segments
            {
                tx_frame!!.num_bytes_curr_segment = MAX_TRANSPORT_PAYLOAD_LEN
            } else  //If entire frame can be sent in one packet
            {
                tx_frame!!.num_bytes_curr_segment = total_bytes
            }
            val msg =
                "Sending " + tx_frame!!.total_bytes.toString() + "bytes in " + tx_frame!!.total_num_seg.toString() + " segments"
            uwbtransportlistener?.onUwbDebugging(msg)
            state = State.XMIT
            sendSegment()
        } else {
            false
        }
    }

    fun nextSegment(): Boolean {
        tx_frame!!.curr_seq++
        return if (tx_frame!!.curr_seq >= tx_frame!!.total_num_seg) //If seq reach last sequence
        {
            println("Done transmitting")
            uwbtransportlistener?.onUwbTXSuccess()
            false
        } else {
            tx_frame!!.curr_byte_idx += MAX_TRANSPORT_PAYLOAD_LEN
            if (tx_frame!!.curr_seq == tx_frame!!.total_num_seg - 1) //If last segment
            {
                val num_bytes_curr_segment = tx_frame!!.total_bytes - tx_frame!!.curr_byte_idx
                if (num_bytes_curr_segment == tx_frame!!.total_bytes % MAX_TRANSPORT_PAYLOAD_LEN) {
                    println("Bytes correct")
                } else {
                    println("Bytes incorrect")
                }
                tx_frame!!.num_bytes_curr_segment = num_bytes_curr_segment
            } else  //Middle segment
            {
                tx_frame!!.num_bytes_curr_segment = MAX_TRANSPORT_PAYLOAD_LEN
            }
            true
        }
    }

    fun sendSegment(): Boolean {
        val transportMsg = ByteArray(MAX_TRANSPORT_LEN)
        transportMsg[TRANSPORT_TYPE_OFFSET] = TRANSPORT_TYPE_RELIABLE
        transportMsg[TRANSPORT_FRAMEID_OFFSET] = tx_frame!!.frame_id.toByte()
        transportMsg[TRANSPORT_NUMSEG_OFFSET] = (tx_frame!!.total_num_seg and 0xFF00 shr 8).toByte()
        transportMsg[TRANSPORT_NUMSEG_OFFSET + 1] = (tx_frame!!.total_num_seg and 0x00FF).toByte()
        transportMsg[TRANSPORT_SEQ_OFFSET] = (tx_frame!!.curr_seq and 0xFF00 shr 8).toByte()
        transportMsg[TRANSPORT_SEQ_OFFSET + 1] = (tx_frame!!.curr_seq and 0x00FF).toByte()
        transportMsg[TRANSPORT_TOTALBYTE_OFFSET] =
            (tx_frame!!.total_bytes and 0xFF00 shr 8).toByte()
        transportMsg[TRANSPORT_TOTALBYTE_OFFSET + 1] = (tx_frame!!.total_bytes and 0x00FF).toByte()
        val num_bytes = tx_frame!!.num_bytes_curr_segment
        transportMsg[TRANSPORT_BYTE_OFFSET] =
            (tx_frame!!.num_bytes_curr_segment and 0xFF00 shr 8).toByte()
        transportMsg[TRANSPORT_BYTE_OFFSET + 1] =
            (tx_frame!!.num_bytes_curr_segment and 0x00FF).toByte()
        System.arraycopy(
            tx_frame!!.data,
            tx_frame!!.curr_byte_idx,
            transportMsg,
            TRANSPORT_DATA_OFFSET,
            num_bytes
        )
        arduino.send(transportMsg)
        return true
    }

    val mainHandler = Handler(Looper.getMainLooper())
    override fun onArduinoMessage(bytes: ByteArray?) {
        val arduino_msg = bytes?.let { String(it) }
        if (bytes != null) {
            if (bytes.size > 0) {
                if (arduino_msg != null) {
                    val wifi = this.context.getSystemService(Context.WIFI_SERVICE) as WifiManager?
                    if (arduino_msg.contains("phone:")) {
                        uwbtransportlistener?.onUwbDebugging(arduino_msg + "\n")
                        // TODO: Turn on WiFi here
                        if (wifi != null && !wifi.isWifiEnabled) {
                            wifi.setWifiEnabled(true)
                            mainHandler.post {
                                try {
                                    Thread.sleep(10000) // sleep for 10 seconds to wait for wifi auto-connection
                                    uwbtransportlistener?.videoDisplay()
                                } catch (e: InterruptedException) {
                                    e.printStackTrace()
                                }
                            }
                        }
                    } else {
//                        Any Other Messages
//                        uwbtransportlistener?.onUwbDebugging("IGNORED: $arduino_msg")
                        return
                    }
                }
            } else {
                return
            }
        }
    }

    override fun onArduinoOpened() {
        state = State.IDLE
        tx_frame = null
        rx_frame = null
//        uwbtransportlistener?.onUwbDebugging("UWB Attached...\n")
    }

    override fun onUsbPermissionDenied() {
//        display("Permission denied... New attempt in 3 sec");
        Handler().postDelayed({ arduino.reopen() }, 3000)
    }

    fun is_rxpacket_uwb_transport(packet: ByteArray): Boolean //TODO: Check if seq 0
    {
        return if (packet.size > TRANSPORT_HEADER_IDX + TRANSPORT_DATA_OFFSET) {
            if (packet[TRANSPORT_HEADER_IDX + TRANSPORT_TYPE_OFFSET] == TRANSPORT_TYPE_RELIABLE) {
                true
            } else {
                false
            }
        } else {
            uwbtransportlistener?.onUwbDebugging("RX Length too short: " + packet.size)
            false
        }
    }

    companion object {
        var ZIGBEE_MSG_TYPE: Byte = 0x41
        var ZIGBEE_ACK_MSG_TYPE: Byte = 0x49
        var TRANSPORT_HEADER_IDX = 11
        var TRANSPORT_TYPE_OFFSET = 0
        var TRANSPORT_FRAMEID_OFFSET = 1
        var TRANSPORT_NUMSEG_OFFSET = 2
        var TRANSPORT_SEQ_OFFSET = 4
        var TRANSPORT_TOTALBYTE_OFFSET = 6
        var TRANSPORT_BYTE_OFFSET = 8
        var TRANSPORT_DATA_OFFSET = 10
        var TRANSPORT_TYPE_RELIABLE: Byte = 0x55
    }
}