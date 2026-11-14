package com.pigmenta.app.ui.camera
import android.Manifest
import android.annotation.SuppressLint
import android.content.Context
import android.content.pm.PackageManager
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.location.Location
import android.location.LocationListener
import android.location.LocationManager
import android.media.AudioManager
import android.net.Uri
import android.os.Bundle
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.camera.core.*
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.content.ContextCompat
import androidx.core.content.FileProvider
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.fragment.app.viewModels
import androidx.navigation.fragment.findNavController
import com.pigmenta.app.R
import com.pigmenta.app.camera.BurstCaptureManager
import com.pigmenta.app.databinding.FragmentCameraBinding
import com.pigmenta.app.model.CaptureData
import com.pigmenta.app.ui.shared.SharedCaptureViewModel
import dagger.hilt.android.AndroidEntryPoint
import java.io.File
import java.text.SimpleDateFormat
import java.util.*
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
@AndroidEntryPoint
class CameraFragment : Fragment(), SensorEventListener {
    private var _binding: FragmentCameraBinding? = null
    private val binding get() = _binding!!
    private val viewModel: CameraViewModel by viewModels()
    private val sharedVm: SharedCaptureViewModel by activityViewModels()
    private var imageCapture: ImageCapture? = null
    private lateinit var cameraExecutor: ExecutorService
    private lateinit var outputDir: File
    private var burstManager: BurstCaptureManager? = null
    private var isBurstRunning = false
    private var cameraProvider: ProcessCameraProvider? = null
    private lateinit var sensorManager: SensorManager
    private var rotationSensor: Sensor? = null
    private val rotationMatrix    = FloatArray(9)
    private val orientationAngles = FloatArray(3)
    private var currentAzimuth: Float? = null
    private var currentPitch: Float? = null
    private var currentRoll: Float? = null
    private var locationManager: LocationManager? = null
    private var lastLocation: Location? = null
    private val locationListener = LocationListener { loc -> lastLocation = loc; updateSensorOverlay() }
    private val audioManager by lazy {
        requireContext().getSystemService(Context.AUDIO_SERVICE) as AudioManager
    }
    private val cameraPermLauncher = registerForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
        if (granted) startCamera() else showPermissionDenied()
    }
    private val locationPermLauncher = registerForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
        if (granted) startLocationUpdates()
    }
    private val galleryLauncher = registerForActivityResult(ActivityResultContracts.GetContent()) { uri: Uri? ->
        uri?.let { sharedVm.updateImageUri(it.toString()); navigateToProcessing(it) }
    }
    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, b: Bundle?): View {
        _binding = FragmentCameraBinding.inflate(inflater, container, false)
        return binding.root
    }
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        cameraExecutor = Executors.newSingleThreadExecutor()
        outputDir = File(requireContext().cacheDir, "captures").also { it.mkdirs() }
        sensorManager = requireContext().getSystemService(Context.SENSOR_SERVICE) as SensorManager
        rotationSensor = sensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR)
        locationManager = requireContext().getSystemService(Context.LOCATION_SERVICE) as LocationManager
        checkCameraPermission()
        checkLocationPermission()
        binding.btnCapture.setOnClickListener { animateShutter(); playClick(); takePhoto() }
        binding.btnGallery.setOnClickListener { galleryLauncher.launch("image/*") }
        binding.btnBurst.setOnClickListener { if (!isBurstRunning) startBurst() }
    }
    override fun onResume() {
        super.onResume()
        rotationSensor?.let { sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_UI) }
    }
    override fun onPause() {
        super.onPause()
        sensorManager.unregisterListener(this)
        locationManager?.removeUpdates(locationListener)
    }
    override fun onDestroyView() {
        super.onDestroyView()
        burstManager?.release()
        cameraExecutor.shutdown()
        _binding = null
    }
    override fun onSensorChanged(event: SensorEvent) {
        if (event.sensor.type == Sensor.TYPE_ROTATION_VECTOR) {
            SensorManager.getRotationMatrixFromVector(rotationMatrix, event.values)
            SensorManager.getOrientation(rotationMatrix, orientationAngles)
            currentAzimuth = Math.toDegrees(orientationAngles[0].toDouble()).toFloat().let { if (it < 0) it + 360f else it }
            currentPitch   = -Math.toDegrees(orientationAngles[1].toDouble()).toFloat()
            currentRoll    = Math.toDegrees(orientationAngles[2].toDouble()).toFloat()
            updateSensorOverlay()
        }
    }
    override fun onAccuracyChanged(sensor: Sensor, accuracy: Int) {}
    private fun checkLocationPermission() {
        if (ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED)
            startLocationUpdates()
        else locationPermLauncher.launch(Manifest.permission.ACCESS_FINE_LOCATION)
    }
    @SuppressLint("MissingPermission")
    private fun startLocationUpdates() {
        try {
            locationManager?.getLastKnownLocation(LocationManager.GPS_PROVIDER)?.let { lastLocation = it }
            locationManager?.requestLocationUpdates(LocationManager.GPS_PROVIDER, 5000L, 5f, locationListener, Looper.getMainLooper())
            locationManager?.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 5000L, 5f, locationListener, Looper.getMainLooper())
        } catch (_: Exception) {}
    }
    private fun updateSensorOverlay() {
        if (_binding == null) return
        val loc = lastLocation
        val gpsText = if (loc != null) {
            val latDir = if (loc.latitude >= 0) "N" else "S"
            val lngDir = if (loc.longitude >= 0) "E" else "O"
            "GPS: %.4f*%s  %.4f*%s  +-%.0fm".format(Math.abs(loc.latitude), latDir, Math.abs(loc.longitude), lngDir, loc.accuracy)
        } else "GPS: buscando..."
        val a = currentAzimuth
        val azimuthText = if (a != null) {
            val dir = when { a<22.5||a>=337.5->"N"; a<67.5->"NE"; a<112.5->"E"; a<157.5->"SE"; a<202.5->"S"; a<247.5->"SO"; a<292.5->"O"; else->"NO" }
            "%.0f* (%s)".format(a, dir)
        } else "Rumbo: -"
        val pitchText = currentPitch?.let { "%+.0f*".format(it) } ?: "Incl: -"
        val surface = CaptureData.inferSurface(currentPitch, currentAzimuth)
        requireActivity().runOnUiThread {
            if (_binding == null) return@runOnUiThread
            binding.tvGps.text     = "\uD83D\uDCCD $gpsText"
            binding.tvAzimuth.text = "\uD83E\uDDED $azimuthText"
            binding.tvPitch.text   = "\uD83D\uDCD0 $pitchText"
            binding.tvSurface.text = "\uD83E\uDEA8 $surface"
        }
    }
    private fun animateShutter() {
        val btn = binding.btnCapture
        btn.animate().scaleX(0.80f).scaleY(0.80f).setDuration(80).withEndAction {
            btn.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#FF4444"))
            btn.animate().scaleX(1f).scaleY(1f).setDuration(150).withEndAction {
                btn.postDelayed({ btn.backgroundTintList = android.content.res.ColorStateList.valueOf(ContextCompat.getColor(requireContext(), R.color.accent_amber)) }, 300)
            }.start()
        }.start()
    }
    private fun playClick() { audioManager.playSoundEffect(AudioManager.FX_KEY_CLICK, 1.0f) }
    private fun checkCameraPermission() {
        if (ContextCompat.checkSelfPermission(requireContext(), Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) startCamera()
        else cameraPermLauncher.launch(Manifest.permission.CAMERA)
    }
    private fun startCamera() {
        val future = ProcessCameraProvider.getInstance(requireContext())
        future.addListener({ cameraProvider = future.get(); bindCameraX() }, ContextCompat.getMainExecutor(requireContext()))
    }
    fun bindCameraX() {
        val provider = cameraProvider ?: return
        val preview = Preview.Builder().build().also { it.setSurfaceProvider(binding.viewFinder.surfaceProvider) }
        imageCapture = ImageCapture.Builder().setCaptureMode(ImageCapture.CAPTURE_MODE_MAXIMIZE_QUALITY).build()
        try {
            provider.unbindAll()
            provider.bindToLifecycle(viewLifecycleOwner, CameraSelector.DEFAULT_BACK_CAMERA, preview, imageCapture)
        } catch (e: Exception) {
            Toast.makeText(requireContext(), getString(R.string.camera_error), Toast.LENGTH_SHORT).show()
        }
    }
    private fun buildCaptureData(imageUri: String): CaptureData {
        val loc = lastLocation
        return CaptureData(
            imageUri = imageUri, timestamp = System.currentTimeMillis(),
            lat = loc?.latitude, lng = loc?.longitude, altitude = loc?.altitude, gpsAccuracy = loc?.accuracy,
            azimuth = currentAzimuth, pitch = currentPitch, roll = currentRoll,
            inferredSurface = CaptureData.inferSurface(currentPitch, currentAzimuth),
        )
    }
    private fun takePhoto() {
        val capture = imageCapture ?: return
        val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())
        val file = File(outputDir, "PIGMENTA_$timestamp.jpg")
        capture.takePicture(ImageCapture.OutputFileOptions.Builder(file).build(), cameraExecutor,
            object : ImageCapture.OnImageSavedCallback {
                override fun onImageSaved(output: ImageCapture.OutputFileResults) {
                    val uri = FileProvider.getUriForFile(requireContext(), "${requireContext().packageName}.provider", file)
                    sharedVm.setCaptureData(buildCaptureData(uri.toString()))
                    viewModel.onPhotoCaptured(uri)
                    requireActivity().runOnUiThread { navigateToProcessing(uri) }
                }
                override fun onError(exc: ImageCaptureException) {
                    requireActivity().runOnUiThread { Toast.makeText(requireContext(), "Error: ${exc.message}", Toast.LENGTH_SHORT).show() }
                }
            })
    }
    private fun startBurst() {
        isBurstRunning = true
        val burstDir = File(outputDir, "burst").also { it.mkdirs() }
        binding.btnBurst.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#FF4444"))
        binding.tvBurstProgress.visibility = View.VISIBLE
        binding.tvBurstProgress.text = "0/9"
        playClick()
        cameraProvider?.unbindAll()
        burstManager?.release()
        burstManager = BurstCaptureManager(
            context = requireContext(), outputDir = burstDir,
            onProgress = { current, total ->
                requireActivity().runOnUiThread { if (_binding != null) binding.tvBurstProgress.text = "$current/$total" }
            },
            onComplete = { files ->
                requireActivity().runOnUiThread {
                    isBurstRunning = false
                    if (_binding != null) {
                        binding.btnBurst.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#2D2D2D"))
                        binding.tvBurstProgress.visibility = View.GONE
                    }
                    val fmt = if (files.firstOrNull()?.name?.endsWith("dng") == true) getString(R.string.burst_raw_label) else getString(R.string.burst_yuv_label)
                    Toast.makeText(requireContext(), getString(R.string.burst_done, files.size) + " [$fmt]", Toast.LENGTH_LONG).show()
                    bindCameraX()
                    files.firstOrNull()?.let { firstFile ->
                        val uri = FileProvider.getUriForFile(requireContext(), "${requireContext().packageName}.provider", firstFile)
                        sharedVm.setCaptureData(buildCaptureData(uri.toString()))
                        navigateToProcessing(uri)
                    }
                }
            },
            onError = { msg ->
                requireActivity().runOnUiThread {
                    isBurstRunning = false
                    if (_binding != null) {
                        binding.btnBurst.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#2D2D2D"))
                        binding.tvBurstProgress.visibility = View.GONE
                    }
                    Toast.makeText(requireContext(), getString(R.string.burst_error, msg), Toast.LENGTH_SHORT).show()
                    bindCameraX()
                }
            }
        )
        burstManager!!.startBurst(9)
    }
    private fun navigateToProcessing(uri: Uri) {
        findNavController().navigate(R.id.action_camera_to_processing, Bundle().apply { putString("imageUri", uri.toString()) })
    }
    private fun showPermissionDenied() {
        Toast.makeText(requireContext(), getString(R.string.camera_permission_denied), Toast.LENGTH_LONG).show()
    }
}
