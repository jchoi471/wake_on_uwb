package com.mci.wakeonuwb

interface UWBtransportListener {
    fun onUwbTXSuccess()
    fun onUwbTXFail(tx_err_msg: String?)
    fun onUwbDebugging(msg: String?)
    fun videoDisplay()
}