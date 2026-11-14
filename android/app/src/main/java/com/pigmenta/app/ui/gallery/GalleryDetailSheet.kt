package com.pigmenta.app.ui.gallery

import android.content.ContentValues
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.MediaStore
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.activityViewModels
import androidx.navigation.fragment.findNavController
import coil.load
import com.google.android.material.bottomsheet.BottomSheetDialogFragment
import com.pigmenta.app.R
import com.pigmenta.app.data.local.ImageEntity
import com.pigmenta.app.data.repository.ImageRepository
import com.pigmenta.app.databinding.BottomSheetGalleryDetailBinding
import com.pigmenta.app.model.CaptureData
import com.pigmenta.app.ui.shared.SharedCaptureViewModel
import dagger.hilt.android.AndroidEntryPoint
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.text.SimpleDateFormat
import java.util.*
import javax.inject.Inject

@AndroidEntryPoint
class GalleryDetailSheet : BottomSheetDialogFragment() {

    @Inject lateinit var repository: ImageRepository
    private val sharedVm: SharedCaptureViewModel by activityViewModels()

    private var _binding: BottomSheetGalleryDetailBinding? = null
    private val binding get() = _binding!!

    private lateinit var entity: ImageEntity

    companion object {
        private const val ARG_ID = "entity_id"
        fun newInstance(id: Long) = GalleryDetailSheet().apply {
            arguments = Bundle().apply { putLong(ARG_ID, id) }
        }
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, b: Bundle?): View {
        _binding = BottomSheetGalleryDetailBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        val id = arguments?.getLong(ARG_ID) ?: return dismiss()

        CoroutineScope(Dispatchers.IO).launch {
            val e = repository.findById(id) ?: return@launch
            withContext(Dispatchers.Main) {
                if (_binding == null) return@withContext
                entity = e
                bindData(e)
            }
        }
    }

    private fun bindData(e: ImageEntity) {
        binding.ivDetail.load(File(e.processedPath)) { crossfade(true) }
        binding.tvFilterName.text = e.filterName
        binding.tvDate.text = SimpleDateFormat("dd/MM/yyyy  HH:mm", Locale.getDefault()).format(Date(e.createdAt))

        // GPS
        val gpsText = if (e.latitude != 0.0 || e.longitude != 0.0) {
            val latDir = if (e.latitude >= 0) "N" else "S"
            val lngDir = if (e.longitude >= 0) "E" else "O"
            "📍 %.4f°%s  %.4f°%s".format(Math.abs(e.latitude), latDir, Math.abs(e.longitude), lngDir) +
                (if (e.altitude != 0.0) "\n    Alt: %.0fm".format(e.altitude) else "")
        } else "📍 GPS: —"
        binding.tvMetaGps.text = gpsText

        // Superficie
        binding.tvMetaSurface.text = if (e.inferredSurface.isNotEmpty()) "🪨 ${e.inferredSurface}" else ""
        binding.tvMetaSurface.visibility = if (e.inferredSurface.isNotEmpty()) View.VISIBLE else View.GONE

        // Orientación
        val orient = buildString {
            if (e.azimuth >= 0) append("🧭 %.0f°  ".format(e.azimuth))
            if (e.pitch > -999) append("📐 %+.0f°  ".format(e.pitch))
        }
        binding.tvMetaOrientation.text = orient.trim()
        binding.tvMetaOrientation.visibility = if (orient.isNotBlank()) View.VISIBLE else View.GONE

        // Notas
        val notesText = buildString {
            if (e.siteLabel.isNotEmpty()) appendLine("📌 ${e.siteLabel}")
            if (e.researcher.isNotEmpty()) appendLine("👤 ${e.researcher}")
            if (e.notes.isNotEmpty()) appendLine("📝 ${e.notes}")
        }.trim()
        if (notesText.isNotEmpty()) {
            binding.tvMetaNotes.text = notesText
            binding.tvMetaNotes.visibility = View.VISIBLE
        }

        // Botones
        binding.btnDownloadImg.setOnClickListener { downloadImage(e) }
        binding.btnDownloadAll.setOnClickListener { downloadAll(e) }
        binding.btnReedit.setOnClickListener { reEdit(e) }
    }

    private fun downloadImage(e: ImageEntity) {
        CoroutineScope(Dispatchers.IO).launch {
            try {
                val bmp = android.graphics.BitmapFactory.decodeFile(e.processedPath) ?: return@launch
                val uri = saveImageToGallery(bmp, "Pigmenta_${e.filterName}_${e.id}")
                withContext(Dispatchers.Main) {
                    if (uri != null) Toast.makeText(requireContext(), getString(R.string.gallery_detail_saved), Toast.LENGTH_SHORT).show()
                }
            } catch (_: Exception) {}
        }
    }

    private fun saveImageToGallery(bmp: android.graphics.Bitmap, name: String): Uri? {
        val ctx = requireContext()
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            val values = ContentValues().apply {
                put(MediaStore.Images.Media.DISPLAY_NAME, "$name.jpg")
                put(MediaStore.Images.Media.MIME_TYPE, "image/jpeg")
                put(MediaStore.Images.Media.RELATIVE_PATH, "Pictures/Pigmenta")
                put(MediaStore.Images.Media.IS_PENDING, 1)
            }
            val uri = ctx.contentResolver.insert(MediaStore.Images.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY), values) ?: return null
            ctx.contentResolver.openOutputStream(uri)?.use { bmp.compress(android.graphics.Bitmap.CompressFormat.JPEG, 95, it) }
            values.clear(); values.put(MediaStore.Images.Media.IS_PENDING, 0)
            ctx.contentResolver.update(uri, values, null, null)
            uri
        } else null
    }

    private fun downloadAll(e: ImageEntity) {
        val text = repository.buildMetadataText(e)
        val cb = requireContext().getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
        cb.setPrimaryClip(ClipData.newPlainText("Ficha Pigmenta", text))
        // También exportar imagen
        downloadImage(e)
        Toast.makeText(requireContext(), getString(R.string.gallery_detail_all_copied), Toast.LENGTH_SHORT).show()
    }

    private fun reEdit(e: ImageEntity) {
        val uri = try {
            androidx.core.content.FileProvider.getUriForFile(
                requireContext(), "${requireContext().packageName}.provider", File(e.processedPath)
            )
        } catch (_: Exception) {
            Uri.parse(e.processedPath)
        }
        // Repoblar SharedVm con los datos de la sesión guardada
        sharedVm.setCaptureData(CaptureData(
            imageUri = e.originalUri.ifEmpty { e.processedPath },
            lat = if (e.latitude != 0.0) e.latitude else null,
            lng = if (e.longitude != 0.0) e.longitude else null,
            altitude = if (e.altitude != 0.0) e.altitude else null,
            gpsAccuracy = if (e.gpsAccuracy > 0) e.gpsAccuracy else null,
            azimuth = if (e.azimuth >= 0) e.azimuth else null,
            pitch = if (e.pitch > -999) e.pitch else null,
            roll = if (e.roll > -999) e.roll else null,
            inferredSurface = e.inferredSurface,
            appliedFilter = e.filterName,
        ))
        dismiss()
        findNavController().navigate(
            R.id.action_gallery_to_processing,
            Bundle().apply { putString("imageUri", uri.toString()) }
        )
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}

