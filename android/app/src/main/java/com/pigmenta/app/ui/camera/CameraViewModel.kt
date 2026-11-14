package com.pigmenta.app.ui.camera

import android.net.Uri
import androidx.lifecycle.ViewModel
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import javax.inject.Inject

@HiltViewModel
class CameraViewModel @Inject constructor() : ViewModel() {

    private val _capturedUri = MutableStateFlow<Uri?>(null)
    val capturedUri: StateFlow<Uri?> = _capturedUri

    fun onPhotoCaptured(uri: Uri) {
        _capturedUri.value = uri
    }
}
