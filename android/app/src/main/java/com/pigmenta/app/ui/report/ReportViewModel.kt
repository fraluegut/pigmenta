package com.pigmenta.app.ui.report

import android.graphics.Bitmap
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.pigmenta.app.data.repository.ImageRepository
import com.pigmenta.app.engine.FilterPreset
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.launch
import javax.inject.Inject

@HiltViewModel
class ReportViewModel @Inject constructor(
    private val repository: ImageRepository,
) : ViewModel() {

    fun save(
        bitmap: Bitmap,
        originalUri: String,
        filterName: String,
        presetId: Int,
        intensity: Float,
        lat: Double, lng: Double, altitude: Double, gpsAccuracy: Float,
        azimuth: Float, pitch: Float, roll: Float,
        inferredSurface: String,
        siteLabel: String, researcher: String, notes: String,
        onResult: (Boolean) -> Unit,
    ) {
        viewModelScope.launch {
            try {
                // Encontrar el preset por ID (o usar un genérico)
                val preset = FilterPreset.allPresets().find { it.id == presetId }
                    ?: FilterPreset.Original

                repository.saveSession(
                    bitmap = bitmap,
                    originalUri = originalUri,
                    preset = preset,
                    intensity = intensity,
                    lat = lat, lng = lng, altitude = altitude, gpsAccuracy = gpsAccuracy,
                    azimuth = azimuth, pitch = pitch, roll = roll,
                    inferredSurface = inferredSurface,
                    siteLabel = siteLabel,
                    researcher = researcher,
                    notes = notes,
                )
                onResult(true)
            } catch (e: Exception) {
                onResult(false)
            }
        }
    }
}

