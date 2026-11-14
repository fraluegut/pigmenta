package com.pigmenta.app.camera

import android.Manifest
import android.content.ContentValues
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.ImageFormat
import android.graphics.Rect
import android.graphics.YuvImage
import android.hardware.camera2.*
import android.hardware.camera2.params.OutputConfiguration
import android.hardware.camera2.params.SessionConfiguration
import android.media.Image
import android.media.ImageReader
import android.os.Build
import android.os.Environment
import android.os.Handler
import android.os.HandlerThread
import android.provider.MediaStore
import android.util.Log
import androidx.core.content.ContextCompat
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.FileOutputStream
import java.text.SimpleDateFormat
import java.util.*
import java.util.concurrent.Executor
import java.util.concurrent.Semaphore
import java.util.concurrent.TimeUnit

/**
 * Captura burst con Camera2. Guarda cada frame como JPEG en la galería del dispositivo.
 * Usa RAW_SENSOR si el dispositivo lo soporta, si no YUV_420_888 → convierte a JPEG.
 */
class BurstCaptureManager(
    private val context: Context,
    private val outputDir: File,
    private val onProgress: (current: Int, total: Int) -> Unit,
    private val onComplete: (files: List<File>) -> Unit,
    private val onError: (message: String) -> Unit,
) {
    companion object {
        private const val TAG = "BurstCapture"
        private const val FRAME_COUNT = 9
        private const val CAMERA_OPEN_TIMEOUT_MS = 2500L
    }

    private val cameraManager = context.getSystemService(Context.CAMERA_SERVICE) as CameraManager
    private val handlerThread = HandlerThread("BurstThread").also { it.start() }
    private val handler = Handler(handlerThread.looper)
    private val executor = Executor { handler.post(it) }

    private var cameraDevice: CameraDevice? = null
    private var captureSession: CameraCaptureSession? = null
    private var imageReader: ImageReader? = null
    private val openLock = Semaphore(1)

    private val capturedFiles = mutableListOf<File>()
    private var totalFrames = FRAME_COUNT
    private var framesReceived = 0
    private var burstFormat = ImageFormat.YUV_420_888
    private var imageWidth = 0
    private var imageHeight = 0

    fun startBurst(frameCount: Int = FRAME_COUNT) {
        totalFrames = frameCount
        framesReceived = 0
        capturedFiles.clear()

        val cameraId = getBackCameraId() ?: run { onError("No se encontró cámara trasera"); return }
        val characteristics = cameraManager.getCameraCharacteristics(cameraId)
        burstFormat = chooseBestFormat(characteristics)

        val map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)!!
        // Para YUV usar resolución media para mayor velocidad de burst
        val sizes = map.getOutputSizes(burstFormat)
        val size = if (burstFormat == ImageFormat.RAW_SENSOR) {
            sizes.maxByOrNull { it.width * it.height }!!
        } else {
            // Máx 12MP para YUV en burst
            sizes.filter { it.width * it.height <= 12_000_000 }
                .maxByOrNull { it.width * it.height } ?: sizes[0]
        }
        imageWidth = size.width
        imageHeight = size.height

        Log.d(TAG, "Burst: format=${if (burstFormat == ImageFormat.RAW_SENSOR) "RAW" else "YUV"} ${size.width}×${size.height}")

        imageReader = ImageReader.newInstance(size.width, size.height, burstFormat, frameCount + 2)
        imageReader!!.setOnImageAvailableListener({ reader ->
            val image = reader.acquireNextImage() ?: return@setOnImageAvailableListener
            saveImage(image)
        }, handler)

        openCamera(cameraId)
    }

    fun release() {
        captureSession?.close(); captureSession = null
        cameraDevice?.close(); cameraDevice = null
        imageReader?.close(); imageReader = null
        handlerThread.quitSafely()
    }

    // ── Camera2 ──────────────────────────────────────────────────────────────

    private fun openCamera(cameraId: String) {
        if (ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA)
            != PackageManager.PERMISSION_GRANTED) { onError("Permiso de cámara no concedido"); return }

        if (!openLock.tryAcquire(CAMERA_OPEN_TIMEOUT_MS, TimeUnit.MILLISECONDS)) {
            onError("Timeout abriendo cámara"); return
        }
        try {
            cameraManager.openCamera(cameraId, executor, object : CameraDevice.StateCallback() {
                override fun onOpened(camera: CameraDevice) {
                    openLock.release(); cameraDevice = camera; createCaptureSession(camera)
                }
                override fun onDisconnected(camera: CameraDevice) {
                    openLock.release(); camera.close(); cameraDevice = null
                }
                override fun onError(camera: CameraDevice, error: Int) {
                    openLock.release(); camera.close(); cameraDevice = null
                    onError("Error al abrir cámara: $error")
                }
            })
        } catch (e: Exception) { openLock.release(); onError("Excepción al abrir cámara: ${e.message}") }
    }

    private fun createCaptureSession(camera: CameraDevice) {
        val reader = imageReader ?: return
        val cb = object : CameraCaptureSession.StateCallback() {
            override fun onConfigured(session: CameraCaptureSession) {
                captureSession = session; lockAfAndCapture(camera, session)
            }
            override fun onConfigureFailed(session: CameraCaptureSession) {
                onError("Fallo al configurar sesión de cámara")
            }
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            camera.createCaptureSession(
                SessionConfiguration(SessionConfiguration.SESSION_REGULAR,
                    listOf(OutputConfiguration(reader.surface)), executor, cb))
        } else {
            @Suppress("DEPRECATION")
            camera.createCaptureSession(listOf(reader.surface), cb, handler)
        }
    }

    private fun lockAfAndCapture(camera: CameraDevice, session: CameraCaptureSession) {
        try {
            val preRequest = camera.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE).apply {
                addTarget(imageReader!!.surface)
                set(CaptureRequest.CONTROL_MODE, CaptureRequest.CONTROL_MODE_AUTO)
                set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE)
                set(CaptureRequest.CONTROL_AF_TRIGGER, CaptureRequest.CONTROL_AF_TRIGGER_START)
                set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON)
                set(CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER, CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER_START)
            }
            session.capture(preRequest.build(), object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(s: CameraCaptureSession, r: CaptureRequest, result: TotalCaptureResult) {
                    handler.postDelayed({ sendBurst(camera, session, result) }, 300)
                }
            }, handler)
        } catch (e: Exception) { onError("Error en AF lock: ${e.message}") }
    }

    private fun sendBurst(camera: CameraDevice, session: CameraCaptureSession, lastResult: TotalCaptureResult) {
        try {
            val requests = (1..totalFrames).map {
                camera.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE).apply {
                    addTarget(imageReader!!.surface)
                    set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_OFF)
                    lastResult.get(CaptureResult.LENS_FOCUS_DISTANCE)?.let { set(CaptureRequest.LENS_FOCUS_DISTANCE, it) }
                    val iso = lastResult.get(CaptureResult.SENSOR_SENSITIVITY)
                    val expTime = lastResult.get(CaptureResult.SENSOR_EXPOSURE_TIME)
                    if (iso != null && expTime != null) {
                        set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_OFF)
                        set(CaptureRequest.SENSOR_SENSITIVITY, iso)
                        set(CaptureRequest.SENSOR_EXPOSURE_TIME, expTime)
                    } else {
                        set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON)
                    }
                }.build()
            }
            session.captureBurst(requests, object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureFailed(s: CameraCaptureSession, r: CaptureRequest, f: CaptureFailure) {
                    Log.w(TAG, "Frame fallido: ${f.reason}")
                }
            }, handler)
        } catch (e: Exception) { onError("Error en burst: ${e.message}") }
    }

    // ── Guardar imagen como JPEG ──────────────────────────────────────────────

    private fun saveImage(image: Image) {
        try {
            val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss_SSS", Locale.US).format(Date())
            val jpegBytes = when (burstFormat) {
                ImageFormat.RAW_SENSOR -> image.planes[0].buffer.let { buf ->
                    ByteArray(buf.remaining()).also { buf.get(it) }
                }
                else -> yuvToJpeg(image)
            }
            image.close()

            // Guardar como JPEG en cache (para navegación interna) Y en galería
            val cacheFile = File(outputDir, "BURST_${timestamp}.jpg")
            cacheFile.writeBytes(jpegBytes)
            exportJpegToGallery(jpegBytes, "Pigmenta_Burst_${timestamp}")

            synchronized(capturedFiles) {
                capturedFiles.add(cacheFile)
                framesReceived++
                onProgress(framesReceived, totalFrames)
                if (framesReceived >= totalFrames) {
                    val saved = capturedFiles.toList()
                    release()
                    onComplete(saved)
                }
            }
        } catch (e: Exception) {
            try { image.close() } catch (_: Exception) {}
            Log.e(TAG, "Error guardando frame", e)
        }
    }

    /** Convierte YUV_420_888 a JPEG respetando rowStride y pixelStride */
    private fun yuvToJpeg(image: Image): ByteArray {
        val w = image.width
        val h = image.height
        val yPlane = image.planes[0]
        val uPlane = image.planes[1]
        val vPlane = image.planes[2]

        val yBuf = yPlane.buffer
        val uBuf = uPlane.buffer
        val vBuf = vPlane.buffer

        val yRowStride = yPlane.rowStride
        val uvRowStride = uPlane.rowStride
        val uvPixelStride = uPlane.pixelStride

        val nv21 = ByteArray(w * h + (w / 2) * (h / 2) * 2)

        // Y plane
        for (row in 0 until h) {
            yBuf.position(row * yRowStride)
            yBuf.get(nv21, row * w, minOf(w, yBuf.remaining()))
        }

        // VU interleaved (NV21)
        val uvOffset = w * h
        for (row in 0 until h / 2) {
            for (col in 0 until w / 2) {
                val uvPos = row * uvRowStride + col * uvPixelStride
                if (uvPos < vBuf.limit()) {
                    nv21[uvOffset + row * w + col * 2] = vBuf.get(uvPos)
                }
                if (uvPos < uBuf.limit()) {
                    nv21[uvOffset + row * w + col * 2 + 1] = uBuf.get(uvPos)
                }
            }
        }

        val yuvImage = YuvImage(nv21, ImageFormat.NV21, w, h, null)
        val out = ByteArrayOutputStream()
        yuvImage.compressToJpeg(Rect(0, 0, w, h), 92, out)
        return out.toByteArray()
    }

    /** Exporta bytes JPEG a la galería pública del dispositivo */
    private fun exportJpegToGallery(jpegBytes: ByteArray, name: String) {
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                val values = ContentValues().apply {
                    put(MediaStore.Images.Media.DISPLAY_NAME, "$name.jpg")
                    put(MediaStore.Images.Media.MIME_TYPE, "image/jpeg")
                    put(MediaStore.Images.Media.RELATIVE_PATH, "${Environment.DIRECTORY_PICTURES}/Pigmenta/Burst")
                    put(MediaStore.Images.Media.IS_PENDING, 1)
                }
                val uri = context.contentResolver.insert(
                    MediaStore.Images.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY), values
                ) ?: return
                context.contentResolver.openOutputStream(uri)?.use { it.write(jpegBytes) }
                values.clear()
                values.put(MediaStore.Images.Media.IS_PENDING, 0)
                context.contentResolver.update(uri, values, null, null)
            } else {
                @Suppress("DEPRECATION")
                val dir = File(Environment.getExternalStoragePublicDirectory(
                    Environment.DIRECTORY_PICTURES), "Pigmenta/Burst").also { it.mkdirs() }
                File(dir, "$name.jpg").writeBytes(jpegBytes)
            }
        } catch (e: Exception) {
            Log.w(TAG, "No se pudo exportar a galería: ${e.message}")
        }
    }

    private fun getBackCameraId() = cameraManager.cameraIdList.firstOrNull { id ->
        cameraManager.getCameraCharacteristics(id)
            .get(CameraCharacteristics.LENS_FACING) == CameraCharacteristics.LENS_FACING_BACK
    }

    private fun chooseBestFormat(chars: CameraCharacteristics): Int {
        val caps = chars.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES) ?: return ImageFormat.YUV_420_888
        val map = chars.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP) ?: return ImageFormat.YUV_420_888
        val supportsRaw = caps.contains(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_RAW)
                && map.getOutputSizes(ImageFormat.RAW_SENSOR)?.isNotEmpty() == true
        return if (supportsRaw) ImageFormat.RAW_SENSOR else ImageFormat.YUV_420_888
    }
}
