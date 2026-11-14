package com.pigmenta.app.ui.shared

import android.graphics.Bitmap
import androidx.lifecycle.ViewModel
import com.pigmenta.app.model.CaptureData
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import javax.inject.Inject

/**
 * ViewModel compartido a nivel de Activity.
 * Mantiene los datos de captura (GPS, sensores, URI, filtro, bitmap procesado)
 * mientras el usuario navega por las tres fases: Captura → Filtros → Informe.
 */
@HiltViewModel
class SharedCaptureViewModel @Inject constructor() : ViewModel() {

    private val _captureData = MutableStateFlow(CaptureData())
    val captureData: StateFlow<CaptureData> = _captureData

    /** Bitmap resultante del filtro aplicado (se guarda al pulsar Continuar → en Filtros) */
    var processedBitmap: Bitmap? = null
        private set

    /** Preset ID del filtro aplicado */
    var processedPresetId: Int = 0
        private set

    var processedIntensity: Float = 1f
        private set

    fun setCaptureData(data: CaptureData) {
        _captureData.value = data
    }

    fun updateImageUri(uri: String) {
        _captureData.value = _captureData.value.copy(imageUri = uri)
    }

    fun updateAppliedFilter(filterName: String, presetId: Int = 0, intensity: Float = 1f, bitmap: Bitmap? = null) {
        _captureData.value = _captureData.value.copy(appliedFilter = filterName)
        processedPresetId = presetId
        processedIntensity = intensity
        if (bitmap != null) processedBitmap = bitmap
    }

    fun clearSession() {
        _captureData.value = CaptureData()
        processedBitmap = null
    }
}

