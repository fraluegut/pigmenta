package com.pigmenta.app.ui.processing

import android.graphics.Bitmap
import android.net.Uri
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.pigmenta.app.data.repository.ImageRepository
import com.pigmenta.app.engine.FilterPreset
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import javax.inject.Inject

sealed class ProcessingUiState {
    object Idle : ProcessingUiState()
    object Loading : ProcessingUiState()
    data class Success(val bitmap: Bitmap) : ProcessingUiState()
    data class Error(val message: String) : ProcessingUiState()
    object Saved : ProcessingUiState()
    object Exported : ProcessingUiState()
    object ExportFailed : ProcessingUiState()
}

@HiltViewModel
class ProcessingViewModel @Inject constructor(
    private val repository: ImageRepository,
) : ViewModel() {

    private val _uiState = MutableStateFlow<ProcessingUiState>(ProcessingUiState.Idle)
    val uiState: StateFlow<ProcessingUiState> = _uiState

    private val _selectedPreset = MutableStateFlow<FilterPreset>(
        FilterPreset.Original   // coincide con índice 0 del adapter
    )
    val selectedPreset: StateFlow<FilterPreset> = _selectedPreset

    private val _intensity = MutableStateFlow(1.0f)
    val intensity: StateFlow<Float> = _intensity

    private val _onlineAvailable = MutableStateFlow(false)
    val onlineAvailable: StateFlow<Boolean> = _onlineAvailable

    private var originalBitmap: Bitmap? = null
    private var processJob: Job? = null

    // ── Actions ───────────────────────────────────────────────────────────────

    fun loadImage(uri: Uri) {
        viewModelScope.launch {
            _uiState.value = ProcessingUiState.Loading
            val bmp = repository.loadBitmap(uri)
            if (bmp != null) {
                originalBitmap = bmp
                // Auto-apply first preset so the user sees the filter result immediately
                applyCurrentFilter()
            } else {
                _uiState.value = ProcessingUiState.Error("Cannot load image")
            }
        }
    }

    fun selectPreset(preset: FilterPreset) {
        _selectedPreset.value = preset
        applyCurrentFilter()
    }

    fun setIntensity(value: Float) {
        _intensity.value = value
        applyCurrentFilter()
    }

    fun applyCurrentFilter() {
        val src = originalBitmap ?: return
        processJob?.cancel()
        processJob = viewModelScope.launch {
            _uiState.value = ProcessingUiState.Loading
            try {
                val result = repository.applyFilterOffline(src, _selectedPreset.value, _intensity.value)
                _uiState.value = ProcessingUiState.Success(result)
            } catch (e: Throwable) {
                _uiState.value = ProcessingUiState.Error(e.message ?: "Processing failed")
            }
        }
    }

    fun applyOnline() {
        val src = originalBitmap ?: return
        viewModelScope.launch {
            _uiState.value = ProcessingUiState.Loading
            val result = repository.processOnline(src, _selectedPreset.value, _intensity.value)
            if (result != null) {
                _uiState.value = ProcessingUiState.Success(result)
            } else {
                _uiState.value = ProcessingUiState.Error("Server unavailable — using offline result")
                applyCurrentFilter()
            }
        }
    }

    fun saveCurrentResult(originalUri: String, siteLabel: String = "") {
        val current = (_uiState.value as? ProcessingUiState.Success)?.bitmap ?: return
        viewModelScope.launch {
            repository.saveProcessedImage(
                bitmap = current,
                originalUri = originalUri,
                preset = _selectedPreset.value,
                intensity = _intensity.value,
                siteLabel = siteLabel,
            )
            _uiState.value = ProcessingUiState.Saved
            // Restore the image so the user can continue working
            _uiState.value = ProcessingUiState.Success(current)
        }
    }

    fun exportToGallery() {
        val current = (_uiState.value as? ProcessingUiState.Success)?.bitmap ?: return
        val preset = _selectedPreset.value
        viewModelScope.launch {
            // Also save to app DB
            repository.saveProcessedImage(
                bitmap = current,
                originalUri = "",
                preset = preset,
                intensity = _intensity.value,
            )
            val uri = repository.exportToGallery(current, preset.displayName)
            _uiState.value = if (uri != null) ProcessingUiState.Exported else ProcessingUiState.ExportFailed
            // Restore image
            _uiState.value = ProcessingUiState.Success(current)
        }
    }

    private fun checkOnlineAvailability() {
        // Online server not configured — keep offline-only
        _onlineAvailable.value = false
    }
}
