package com.pigmenta.app.ui.report

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.fragment.app.viewModels
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.pigmenta.app.R
import com.pigmenta.app.databinding.FragmentReportBinding
import com.pigmenta.app.model.CaptureData
import com.pigmenta.app.ui.shared.SharedCaptureViewModel
import dagger.hilt.android.AndroidEntryPoint

@AndroidEntryPoint
class ReportFragment : Fragment() {

    private var _binding: FragmentReportBinding? = null
    private val binding get() = _binding!!

    private val sharedVm: SharedCaptureViewModel by activityViewModels()
    private val viewModel: ReportViewModel by viewModels()

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, b: Bundle?): View {
        _binding = FragmentReportBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        val data = sharedVm.captureData.value
        populateMetadata(data)

        binding.btnSave.setOnClickListener { saveSession(data) }
        binding.btnSend.setOnClickListener { showSendDialog(data) }
    }

    private fun populateMetadata(data: CaptureData) {
        binding.tvDatetime.text    = "ðŸ• ${data.formattedDateTime}"
        binding.tvCoords.text      = "ðŸ“ ${data.formattedCoords}"
        binding.tvAltitude.text    = "    Altitud: ${data.formattedAltitude}  PrecisiÃ³n: ${data.formattedAccuracy}"
        binding.tvAzimuth.text     = "ðŸ§­ Rumbo: ${data.formattedAzimuth}"
        binding.tvSurface.text     = "ðŸª¨ Superficie: ${data.inferredSurface.ifBlank { "â€”" }}"
        binding.tvInclination.text = "ðŸ“ InclinaciÃ³n: ${data.formattedPitch}  Lateral: ${data.formattedRoll}"
        binding.tvFilter.text      = "ðŸ”¬ Filtro: ${data.appliedFilter.ifBlank { "â€”" }}"
    }

    private fun saveSession(data: CaptureData) {
        val bitmap = sharedVm.processedBitmap
        if (bitmap == null) {
            Toast.makeText(requireContext(), getString(R.string.report_no_image), Toast.LENGTH_SHORT).show()
            return
        }
        binding.btnSave.isEnabled = false
        binding.btnSave.text = getString(R.string.report_saving)

        viewModel.save(
            bitmap         = bitmap,
            originalUri    = data.imageUri,
            filterName     = data.appliedFilter,
            presetId       = sharedVm.processedPresetId,
            intensity      = sharedVm.processedIntensity,
            lat            = data.lat ?: 0.0,
            lng            = data.lng ?: 0.0,
            altitude       = data.altitude ?: 0.0,
            gpsAccuracy    = data.gpsAccuracy ?: 0f,
            azimuth        = data.azimuth ?: -1f,
            pitch          = data.pitch ?: -999f,
            roll           = data.roll ?: -999f,
            inferredSurface = data.inferredSurface,
            siteLabel      = binding.etSite.text?.toString()?.trim() ?: "",
            researcher     = binding.etResearcher.text?.toString()?.trim() ?: "",
            notes          = binding.etNotes.text?.toString()?.trim() ?: "",
        ) { success ->
            if (_binding == null) return@save
            binding.btnSave.isEnabled = !success
            binding.btnSave.text = getString(if (success) R.string.report_saved_ok else R.string.report_btn_save)
            val msg = if (success) getString(R.string.report_saved_ok) else getString(R.string.export_failed)
            Toast.makeText(requireContext(), msg, Toast.LENGTH_SHORT).show()
        }
    }

    private fun showSendDialog(data: CaptureData) {
        val summary = buildString {
            appendLine("ðŸ“‹ FICHA DE REGISTRO PIGMENTA")
            appendLine("â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€")
            val site = binding.etSite.text?.toString()?.trim() ?: ""
            val researcher = binding.etResearcher.text?.toString()?.trim() ?: ""
            val notes = binding.etNotes.text?.toString()?.trim() ?: ""
            if (site.isNotEmpty())       appendLine("Yacimiento: $site")
            if (researcher.isNotEmpty()) appendLine("Investigador: $researcher")
            appendLine()
            appendLine("ðŸ• ${data.formattedDateTime}")
            appendLine("ðŸ“ ${data.formattedCoords}")
            if (data.altitude != null)   appendLine("   Alt: ${data.formattedAltitude}  ${data.formattedAccuracy}")
            appendLine("ðŸ§­ ${data.formattedAzimuth}")
            appendLine("ðŸª¨ ${data.inferredSurface.ifBlank { "â€”" }}")
            appendLine("ðŸ“ Incl: ${data.formattedPitch}  Lat: ${data.formattedRoll}")
            appendLine("ðŸ”¬ ${data.appliedFilter.ifBlank { "â€”" }}")
            if (notes.isNotEmpty()) { appendLine(); appendLine("ðŸ“ $notes") }
        }

        MaterialAlertDialogBuilder(requireContext())
            .setTitle(getString(R.string.report_dialog_title))
            .setMessage(getString(R.string.report_dialog_message))
            .setPositiveButton(getString(R.string.report_dialog_ok)) { d, _ -> d.dismiss() }
            .setNeutralButton(getString(R.string.report_dialog_copy)) { _, _ ->
                val cb = requireContext().getSystemService(android.content.Context.CLIPBOARD_SERVICE)
                    as android.content.ClipboardManager
                cb.setPrimaryClip(android.content.ClipData.newPlainText("Ficha Pigmenta", summary))
                com.google.android.material.snackbar.Snackbar
                    .make(binding.root, getString(R.string.report_copied), com.google.android.material.snackbar.Snackbar.LENGTH_SHORT).show()
            }
            .show()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}