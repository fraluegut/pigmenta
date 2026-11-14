package com.pigmenta.app.ui.processing
import android.net.Uri
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.fragment.app.viewModels
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import androidx.navigation.fragment.findNavController
import androidx.recyclerview.widget.LinearLayoutManager
import com.pigmenta.app.R
import com.pigmenta.app.databinding.FragmentProcessingBinding
import com.pigmenta.app.engine.FilterPreset
import com.pigmenta.app.ui.common.FilterAdapter
import com.pigmenta.app.ui.shared.SharedCaptureViewModel
import dagger.hilt.android.AndroidEntryPoint
import kotlinx.coroutines.launch
@AndroidEntryPoint
class ProcessingFragment : Fragment() {
    private var _binding: FragmentProcessingBinding? = null
    private val binding get() = _binding!!
    private val viewModel: ProcessingViewModel by viewModels()
    private val sharedVm: SharedCaptureViewModel by activityViewModels()
    private lateinit var filterAdapter: FilterAdapter
    private var imageUri: String? = null
    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, b: Bundle?): View {
        _binding = FragmentProcessingBinding.inflate(inflater, container, false)
        return binding.root
    }
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        imageUri = arguments?.getString("imageUri")
        imageUri?.let { viewModel.loadImage(Uri.parse(it)) }
        setupFilterList()
        setupControls()
        observeState()
    }
    private fun setupFilterList() {
        filterAdapter = FilterAdapter(FilterPreset.allPresets()) { preset -> viewModel.selectPreset(preset) }
        binding.recyclerFilters.apply {
            layoutManager = LinearLayoutManager(requireContext(), LinearLayoutManager.HORIZONTAL, false)
            adapter = filterAdapter
        }
    }
    private fun setupControls() {
        binding.sliderIntensity.addOnChangeListener { _, value, _ -> viewModel.setIntensity(value) }
        binding.btnDiscard.setOnClickListener { findNavController().popBackStack() }
        binding.btnContinue.setOnClickListener {
            val preset = viewModel.selectedPreset.value
            val bitmap = (viewModel.uiState.value as? ProcessingUiState.Success)?.bitmap
            sharedVm.updateAppliedFilter(
                filterName = preset.displayName,
                presetId   = preset.id,
                intensity  = viewModel.intensity.value,
                bitmap     = bitmap,
            )
            findNavController().navigate(R.id.action_processing_to_report)
        }
    }
    private fun observeState() {
        viewLifecycleOwner.lifecycleScope.launch {
            repeatOnLifecycle(Lifecycle.State.STARTED) {
                launch {
                    viewModel.uiState.collect { state ->
                        when (state) {
                            is ProcessingUiState.Idle    -> binding.progressBar.visibility = View.GONE
                            is ProcessingUiState.Loading -> binding.progressBar.visibility = View.VISIBLE
                            is ProcessingUiState.Success -> { binding.progressBar.visibility = View.GONE; binding.imagePreview.setImageBitmap(state.bitmap) }
                            is ProcessingUiState.Error   -> { binding.progressBar.visibility = View.GONE; Toast.makeText(requireContext(), state.message, Toast.LENGTH_SHORT).show() }
                            is ProcessingUiState.Saved   -> binding.progressBar.visibility = View.GONE
                            is ProcessingUiState.Exported -> binding.progressBar.visibility = View.GONE
                            is ProcessingUiState.ExportFailed -> { binding.progressBar.visibility = View.GONE; Toast.makeText(requireContext(), getString(R.string.export_failed), Toast.LENGTH_SHORT).show() }
                        }
                    }
                }
                launch {
                    viewModel.selectedPreset.collect { preset ->
                        binding.tvFilterName.text = preset.displayName
                        binding.tvFilterDesc.text = preset.description
                    }
                }
            }
        }
    }
    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
