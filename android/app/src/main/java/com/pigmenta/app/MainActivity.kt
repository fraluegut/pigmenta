package com.pigmenta.app

import android.graphics.Color
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.navigation.NavController
import androidx.navigation.fragment.NavHostFragment
import androidx.navigation.ui.setupWithNavController
import com.pigmenta.app.databinding.ActivityMainBinding
import dagger.hilt.android.AndroidEntryPoint

@AndroidEntryPoint
class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var navController: NavController

    // Destinos donde se oculta toolbar y bottom nav
    private val fullScreenDestinations = setOf(R.id.welcomeFragment)
    private val phaseDestinations = setOf(R.id.cameraFragment, R.id.processingFragment, R.id.reportFragment)

    private val colorAmber  = Color.parseColor("#FFCC44")
    private val colorMuted  = Color.parseColor("#666666")
    private val barAmber    = Color.parseColor("#FFFFCC44")
    private val barMuted    = Color.parseColor("#33FFFFFF")

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val navHostFragment =
            supportFragmentManager.findFragmentById(R.id.nav_host_fragment) as NavHostFragment
        navController = navHostFragment.navController

        binding.bottomNav.setupWithNavController(navController)

        // Toolbar menu → Acerca de
        binding.toolbar.setOnMenuItemClickListener { item ->
            when (item.itemId) {
                R.id.action_credits -> {
                    navController.navigate(R.id.creditsFragment)
                    true
                }
                else -> false
            }
        }

        // Título dinámico + ocultar chrome en welcome
        navController.addOnDestinationChangedListener { _, destination, _ ->
            val isFullScreen = destination.id in fullScreenDestinations
            val isPhase      = destination.id in phaseDestinations

            val chrome = if (isFullScreen) View.GONE else View.VISIBLE
            binding.toolbar.visibility  = chrome
            binding.bottomNav.visibility = chrome
            binding.phaseBar.visibility = if (isPhase && !isFullScreen) View.VISIBLE else View.GONE

            // Actualizar step activo
            if (isPhase) updatePhaseBar(destination.id)

            binding.toolbar.title = when (destination.id) {
                R.id.cameraFragment,
                R.id.processingFragment,
                R.id.reportFragment -> getString(R.string.app_name)
                R.id.galleryFragment   -> getString(R.string.nav_gallery)
                R.id.infoFragment      -> getString(R.string.nav_info)
                R.id.creditsFragment   -> getString(R.string.nav_credits)
                else                   -> getString(R.string.app_name)
            }
        }
    }

    private fun updatePhaseBar(destId: Int) {
        val activePhase = when (destId) {
            R.id.cameraFragment     -> 1
            R.id.processingFragment -> 2
            R.id.reportFragment     -> 3
            else                    -> 0
        }
        listOf(
            Triple(binding.tvPhase1, binding.linePhase1, 1),
            Triple(binding.tvPhase2, binding.linePhase2, 2),
            Triple(binding.tvPhase3, binding.linePhase3, 3),
        ).forEach { (tv, line, phase) ->
            val active = phase == activePhase
            val done   = phase < activePhase
            tv.setTextColor(if (active) colorAmber else colorMuted)
            line.setBackgroundColor(if (active) barAmber else if (done) Color.parseColor("#88FFCC44") else barMuted)
        }
    }
}
