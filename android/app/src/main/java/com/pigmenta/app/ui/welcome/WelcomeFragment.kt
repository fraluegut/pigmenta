package com.pigmenta.app.ui.welcome

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.navigation.fragment.findNavController
import com.pigmenta.app.R
import com.pigmenta.app.databinding.FragmentWelcomeBinding

class WelcomeFragment : Fragment() {

    private var _binding: FragmentWelcomeBinding? = null
    private val binding get() = _binding!!

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, b: Bundle?): View {
        _binding = FragmentWelcomeBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        // Animación de entrada suave
        binding.ivLogo.alpha = 0f
        binding.tvAppName.alpha = 0f
        binding.tvTagline.alpha = 0f
        binding.tvDescription.alpha = 0f
        binding.chipsRow.alpha = 0f
        binding.btnEnter.alpha = 0f

        view.post {
            binding.ivLogo.animate().alpha(1f).translationYBy(-20f).setDuration(600).setStartDelay(100).start()
            binding.tvAppName.animate().alpha(1f).setDuration(500).setStartDelay(300).start()
            binding.tvTagline.animate().alpha(1f).setDuration(500).setStartDelay(450).start()
            binding.tvDescription.animate().alpha(1f).setDuration(500).setStartDelay(600).start()
            binding.chipsRow.animate().alpha(1f).setDuration(500).setStartDelay(750).start()
            binding.btnEnter.animate().alpha(1f).scaleX(1f).scaleY(1f).setDuration(600).setStartDelay(900).start()
        }

        binding.btnEnter.setOnClickListener {
            findNavController().navigate(R.id.action_welcome_to_camera)
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}

