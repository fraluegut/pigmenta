package com.pigmenta.app.model

import java.text.SimpleDateFormat
import java.util.*

/**
 * Toda la información recogida en el momento de la captura:
 * localización GPS, orientación del dispositivo (rumbo/inclinación) y metadatos.
 * Se pasa entre fragmentos mediante [SharedCaptureViewModel].
 */
data class CaptureData(
    // ── Imagen ──────────────────────────────────────────────────────────────
    val imageUri: String = "",
    val timestamp: Long = System.currentTimeMillis(),

    // ── GPS ─────────────────────────────────────────────────────────────────
    val lat: Double? = null,
    val lng: Double? = null,
    val altitude: Double? = null,
    val gpsAccuracy: Float? = null,       // metros

    // ── Orientación (SensorManager ROTATION_VECTOR) ──────────────────────
    val azimuth: Float? = null,           // rumbo brújula 0–360°
    val pitch: Float? = null,             // inclinación vertical −90…+90°
    val roll: Float? = null,              // balanceo lateral −180…+180°

    // ── Inferencia de superficie ─────────────────────────────────────────
    val inferredSurface: String = "",

    // ── Datos añadidos tras el filtro ────────────────────────────────────
    val appliedFilter: String = "",
) {

    /** Fecha y hora legible: "2026-04-21  14:32:05" */
    val formattedDateTime: String
        get() = SimpleDateFormat("yyyy-MM-dd  HH:mm:ss", Locale.getDefault())
            .format(Date(timestamp))

    /** Coordenadas formateadas: "40.4168° N  3.7038° O" (o "—" si no disponibles) */
    val formattedCoords: String
        get() = if (lat != null && lng != null) {
            val latDir = if (lat >= 0) "N" else "S"
            val lngDir = if (lng >= 0) "E" else "O"
            "%.4f° %s  %.4f° %s".format(Math.abs(lat), latDir, Math.abs(lng), lngDir)
        } else "—"

    /** Altitud formateada: "892 m" o "—" */
    val formattedAltitude: String
        get() = altitude?.let { "%.0f m".format(it) } ?: "—"

    /** Precisión GPS: "±15 m" o "—" */
    val formattedAccuracy: String
        get() = gpsAccuracy?.let { "±%.0f m".format(it) } ?: "—"

    /** Rumbo formateado: "276° (O)" o "—" */
    val formattedAzimuth: String
        get() = azimuth?.let {
            val a = ((it % 360) + 360) % 360
            val dir = when {
                a < 22.5 || a >= 337.5 -> "N"
                a < 67.5  -> "NE"
                a < 112.5 -> "E"
                a < 157.5 -> "SE"
                a < 202.5 -> "S"
                a < 247.5 -> "SO"
                a < 292.5 -> "O"
                else       -> "NO"
            }
            "%.0f° (%s)".format(a, dir)
        } ?: "—"

    /** Inclinación formateada: "+32°" o "—" */
    val formattedPitch: String
        get() = pitch?.let { "%+.0f°".format(it) } ?: "—"

    /** Balanceo lateral: "−5°" o "—" */
    val formattedRoll: String
        get() = roll?.let { "%+.0f°".format(it) } ?: "—"

    companion object {
        /**
         * Infiere en qué superficie está la pintura a partir de la orientación del dispositivo.
         * - pitch > +60° → cámara apuntando al techo → "Techo / Bóveda"
         * - pitch < −60° → cámara apuntando al suelo → "Suelo"
         * - resto          → pared; usar azimuth para saber orientación
         */
        fun inferSurface(pitch: Float?, azimuth: Float?): String {
            pitch ?: return "Desconocida"
            return when {
                pitch > 60f  -> "Techo / Bóveda"
                pitch < -60f -> "Suelo"
                else -> {
                    val a = azimuth ?: return "Pared (rumbo desconocido)"
                    val norm = ((a % 360) + 360) % 360
                    when {
                        norm < 22.5 || norm >= 337.5 -> "Pared Norte"
                        norm < 67.5  -> "Pared Noreste"
                        norm < 112.5 -> "Pared Este"
                        norm < 157.5 -> "Pared Sureste"
                        norm < 202.5 -> "Pared Sur"
                        norm < 247.5 -> "Pared Suroeste"
                        norm < 292.5 -> "Pared Oeste"
                        else          -> "Pared Noroeste"
                    }
                }
            }
        }
    }
}

