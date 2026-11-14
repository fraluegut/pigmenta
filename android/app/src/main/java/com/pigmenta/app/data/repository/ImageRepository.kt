package com.pigmenta.app.data.repository

import android.content.ContentValues
import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Matrix
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.provider.MediaStore
import androidx.exifinterface.media.ExifInterface
import com.pigmenta.app.data.local.ImageDao
import com.pigmenta.app.data.local.ImageEntity
import com.pigmenta.app.data.remote.PigmentaApiService
import com.pigmenta.app.data.remote.dto.PigmentAnalysisResponse
import com.pigmenta.app.engine.FilterPreset
import com.pigmenta.app.engine.PigmentaNative
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.withContext
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.MultipartBody
import okhttp3.RequestBody.Companion.toRequestBody
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import javax.inject.Inject
import javax.inject.Singleton

@Singleton
class ImageRepository @Inject constructor(
    @ApplicationContext private val context: Context,
    private val dao: ImageDao,
    private val api: PigmentaApiService,
) {

    val savedImages: Flow<List<ImageEntity>> = dao.observeAll()

    // ── Offline processing ────────────────────────────────────────────────────

    /**
     * Load a [Bitmap] from a content/file [uri].
     */
    suspend fun loadBitmap(uri: Uri): Bitmap? = withContext(Dispatchers.IO) {
        try {
            // 1. Decode with inSampleSize to avoid OOM on large camera photos
            val opts = BitmapFactory.Options().apply { inJustDecodeBounds = true }
            context.contentResolver.openInputStream(uri)?.use { BitmapFactory.decodeStream(it, null, opts) }

            val maxPx = 1920
            val longest = maxOf(opts.outWidth, opts.outHeight)
            opts.inSampleSize = if (longest > maxPx) {
                var s = 1
                while (longest / (s * 2) >= maxPx) s *= 2
                s
            } else 1
            opts.inJustDecodeBounds = false

            val raw = context.contentResolver.openInputStream(uri)?.use { stream ->
                BitmapFactory.decodeStream(stream, null, opts)
            } ?: return@withContext null

            // 2. Correct EXIF orientation
            val rotated = context.contentResolver.openInputStream(uri)?.use { stream ->
                val exif = ExifInterface(stream)
                val rotation = when (exif.getAttributeInt(
                    ExifInterface.TAG_ORIENTATION, ExifInterface.ORIENTATION_NORMAL
                )) {
                    ExifInterface.ORIENTATION_ROTATE_90  -> 90f
                    ExifInterface.ORIENTATION_ROTATE_180 -> 180f
                    ExifInterface.ORIENTATION_ROTATE_270 -> 270f
                    else -> 0f
                }
                if (rotation != 0f) {
                    val m = Matrix().apply { postRotate(rotation) }
                    Bitmap.createBitmap(raw, 0, 0, raw.width, raw.height, m, true)
                } else raw
            } ?: raw

            rotated
        } catch (e: Throwable) {
            null
        }
    }

    /**
     * Apply [preset] to [src] locally (C++ engine, no network needed).
     */
    suspend fun applyFilterOffline(
        src: Bitmap,
        preset: FilterPreset,
        intensity: Float,
    ): Bitmap = withContext(Dispatchers.Default) {
        PigmentaNative.applyPreset(src, preset, intensity)
    }

    /**
     * Save a processed [bitmap] to the app's private files directory and
     * persist metadata in Room.
     */
    /**
     * Guarda la sesión completa: imagen procesada + todos los metadatos de CaptureData
     * + anotaciones del usuario. Retorna la entidad guardada.
     */
    suspend fun saveSession(
        bitmap: Bitmap,
        originalUri: String,
        preset: FilterPreset,
        intensity: Float,
        lat: Double = 0.0,
        lng: Double = 0.0,
        altitude: Double = 0.0,
        gpsAccuracy: Float = 0f,
        azimuth: Float = -1f,
        pitch: Float = -999f,
        roll: Float = -999f,
        inferredSurface: String = "",
        siteLabel: String = "",
        researcher: String = "",
        notes: String = "",
    ): ImageEntity = withContext(Dispatchers.IO) {
        val dir = File(context.filesDir, "processed").also { it.mkdirs() }
        val file = File(dir, "img_${System.currentTimeMillis()}.jpg")
        FileOutputStream(file).use { out ->
            bitmap.compress(Bitmap.CompressFormat.JPEG, 95, out)
        }
        val entity = ImageEntity(
            originalUri = originalUri,
            processedPath = file.absolutePath,
            filterPresetId = preset.id,
            filterName = preset.displayName,
            intensity = intensity,
            siteLabel = siteLabel,
            latitude = lat,
            longitude = lng,
            altitude = altitude,
            gpsAccuracy = gpsAccuracy,
            azimuth = azimuth,
            pitch = pitch,
            roll = roll,
            inferredSurface = inferredSurface,
            researcher = researcher,
            notes = notes,
        )
        val id = dao.insert(entity)
        // También exportar a la galería del dispositivo
        exportToGallery(bitmap, preset.displayName)
        entity.copy(id = id)
    }

    /**
     * Exporta el JSON de metadatos de una entidad como texto plano.
     */
    fun buildMetadataText(entity: ImageEntity): String = buildString {
        appendLine("=== FICHA PIGMENTA ===")
        appendLine("Fecha: ${java.text.SimpleDateFormat("yyyy-MM-dd HH:mm:ss", java.util.Locale.getDefault()).format(java.util.Date(entity.createdAt))}")
        if (entity.siteLabel.isNotEmpty()) appendLine("Yacimiento: ${entity.siteLabel}")
        if (entity.researcher.isNotEmpty()) appendLine("Investigador: ${entity.researcher}")
        appendLine()
        appendLine("--- GPS ---")
        if (entity.latitude != 0.0 || entity.longitude != 0.0) {
            val latDir = if (entity.latitude >= 0) "N" else "S"
            val lngDir = if (entity.longitude >= 0) "E" else "O"
            appendLine("Coordenadas: %.4f°%s  %.4f°%s".format(Math.abs(entity.latitude), latDir, Math.abs(entity.longitude), lngDir))
            if (entity.altitude != 0.0) appendLine("Altitud: %.0f m".format(entity.altitude))
            if (entity.gpsAccuracy > 0) appendLine("Precisión GPS: ±%.0f m".format(entity.gpsAccuracy))
        } else appendLine("GPS: no disponible")
        appendLine()
        appendLine("--- Orientación ---")
        if (entity.azimuth >= 0) appendLine("Rumbo: %.0f°".format(entity.azimuth))
        if (entity.pitch > -999) appendLine("Inclinación: %+.0f°".format(entity.pitch))
        if (entity.roll > -999) appendLine("Lateral: %+.0f°".format(entity.roll))
        if (entity.inferredSurface.isNotEmpty()) appendLine("Superficie: ${entity.inferredSurface}")
        appendLine()
        appendLine("--- Filtro ---")
        appendLine("Filtro aplicado: ${entity.filterName}")
        appendLine("Intensidad: %.0f%%".format(entity.intensity * 100))
        if (entity.notes.isNotEmpty()) {
            appendLine()
            appendLine("--- Observaciones ---")
            appendLine(entity.notes)
        }
    }

    suspend fun saveProcessedImage(
        bitmap: Bitmap,
        originalUri: String,
        preset: FilterPreset,
        intensity: Float,
        siteLabel: String = "",
    ): ImageEntity = withContext(Dispatchers.IO) {
        val dir = File(context.filesDir, "processed").also { it.mkdirs() }
        val file = File(dir, "img_${System.currentTimeMillis()}.png")
        FileOutputStream(file).use { out ->
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, out)
        }
        val entity = ImageEntity(
            originalUri = originalUri,
            processedPath = file.absolutePath,
            filterPresetId = preset.id,
            filterName = preset.displayName,
            intensity = intensity,
            siteLabel = siteLabel,
        )
        val id = dao.insert(entity)
        entity.copy(id = id)
    }

    suspend fun findById(id: Long): ImageEntity? = withContext(Dispatchers.IO) { dao.findById(id) }

    suspend fun deleteImage(entity: ImageEntity) = withContext(Dispatchers.IO) {
        File(entity.processedPath).delete()
        dao.delete(entity)
    }

    /**
     * Export [bitmap] to the device's public Pictures gallery (accessible from the Gallery app).
     * Returns the MediaStore Uri on success, null on failure.
     */
    suspend fun exportToGallery(bitmap: Bitmap, filterName: String): Uri? = withContext(Dispatchers.IO) {
        val fileName = "Pigmenta_${filterName}_${System.currentTimeMillis()}.jpg"
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                // Android 10+ — no WRITE permission needed
                val values = ContentValues().apply {
                    put(MediaStore.Images.Media.DISPLAY_NAME, fileName)
                    put(MediaStore.Images.Media.MIME_TYPE, "image/jpeg")
                    put(MediaStore.Images.Media.RELATIVE_PATH, "${Environment.DIRECTORY_PICTURES}/Pigmenta")
                    put(MediaStore.Images.Media.IS_PENDING, 1)
                }
                val collection = MediaStore.Images.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY)
                val itemUri = context.contentResolver.insert(collection, values) ?: return@withContext null
                context.contentResolver.openOutputStream(itemUri)?.use { out ->
                    bitmap.compress(Bitmap.CompressFormat.JPEG, 95, out)
                }
                values.clear()
                values.put(MediaStore.Images.Media.IS_PENDING, 0)
                context.contentResolver.update(itemUri, values, null, null)
                itemUri
            } else {
                // Android 8/9 — write directly to public Pictures
                @Suppress("DEPRECATION")
                val dir = File(
                    Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES),
                    "Pigmenta"
                ).also { it.mkdirs() }
                val file = File(dir, fileName)
                FileOutputStream(file).use { bitmap.compress(Bitmap.CompressFormat.JPEG, 95, it) }
                // Notify MediaStore
                val values = ContentValues().apply {
                    put(MediaStore.Images.Media.DATA, file.absolutePath)
                    put(MediaStore.Images.Media.MIME_TYPE, "image/jpeg")
                }
                context.contentResolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values)
            }
        } catch (e: Exception) {
            null
        }
    }

    // ── Online processing ─────────────────────────────────────────────────────

    /**
     * Send [src] to the backend for server-side processing.
     * Returns null if the server is unreachable.
     */
    suspend fun processOnline(
        src: Bitmap,
        preset: FilterPreset,
        intensity: Float,
    ): Bitmap? = withContext(Dispatchers.IO) {
        val bytes = bitmapToJpegBytes(src)
        val body = bytes.toRequestBody("image/jpeg".toMediaType())
        val part = MultipartBody.Part.createFormData("file", "image.jpg", body)

        try {
            val response = api.processImage(
                file = part,
                filterType = preset.id.toString().toRequestBody("text/plain".toMediaType()),
                intensity = intensity.toString().toRequestBody("text/plain".toMediaType()),
                targetStd = "45".toRequestBody("text/plain".toMediaType()),
            )
            if (response.isSuccessful) {
                response.body()?.bytes()?.let { BitmapFactory.decodeByteArray(it, 0, it.size) }
            } else null
        } catch (e: Exception) {
            null
        }
    }

    /**
     * Send [src] to the backend for pigment analysis.
     * Returns null on network error.
     */
    suspend fun analyseOnline(
        src: Bitmap,
        preset: FilterPreset,
        nClusters: Int = 5,
    ): PigmentAnalysisResponse? = withContext(Dispatchers.IO) {
        val bytes = bitmapToJpegBytes(src)
        val body = bytes.toRequestBody("image/jpeg".toMediaType())
        val part = MultipartBody.Part.createFormData("file", "image.jpg", body)

        try {
            api.analyseImage(
                file = part,
                nClusters = nClusters.toString().toRequestBody("text/plain".toMediaType()),
                filterType = preset.id.toString().toRequestBody("text/plain".toMediaType()),
                intensity = "1.0".toRequestBody("text/plain".toMediaType()),
            )
        } catch (e: Exception) {
            null
        }
    }

    private fun bitmapToJpegBytes(bitmap: Bitmap, quality: Int = 90): ByteArray {
        val stream = java.io.ByteArrayOutputStream()
        bitmap.compress(Bitmap.CompressFormat.JPEG, quality, stream)
        return stream.toByteArray()
    }
}
