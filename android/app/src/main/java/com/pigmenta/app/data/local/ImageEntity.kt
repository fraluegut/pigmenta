package com.pigmenta.app.data.local

import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "processed_images")
data class ImageEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    // Imágenes
    val originalUri: String,
    val processedPath: String,
    // Filtro
    val filterPresetId: Int,
    val filterName: String,
    val intensity: Float,
    // Timestamp
    val createdAt: Long = System.currentTimeMillis(),
    // GPS
    val siteLabel: String = "",
    val latitude: Double = 0.0,
    val longitude: Double = 0.0,
    val altitude: Double = 0.0,
    val gpsAccuracy: Float = 0f,
    // Orientación
    val azimuth: Float = -1f,          // -1 = desconocido
    val pitch: Float = -999f,          // -999 = desconocido
    val roll: Float = -999f,
    val inferredSurface: String = "",
    // Anotaciones del usuario
    val researcher: String = "",
    val notes: String = "",
)
