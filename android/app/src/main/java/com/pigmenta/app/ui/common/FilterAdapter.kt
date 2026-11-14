package com.pigmenta.app.ui.common

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.pigmenta.app.databinding.ItemFilterPresetBinding
import com.pigmenta.app.engine.FilterPreset

class FilterAdapter(
    private val presets: List<FilterPreset>,
    private val onSelect: (FilterPreset) -> Unit,
) : RecyclerView.Adapter<FilterAdapter.ViewHolder>() {

    private var selectedIndex = 0

    inner class ViewHolder(private val b: ItemFilterPresetBinding) :
        RecyclerView.ViewHolder(b.root) {

        fun bind(preset: FilterPreset, selected: Boolean, position: Int) {
            b.tvName.text = preset.displayName
            b.tvDesc.text = preset.description
            b.root.isSelected = selected
            b.root.strokeWidth = if (selected) 2.dpToPx(b.root.context) else 0
            b.root.setCardBackgroundColor(
                if (selected)
                    androidx.core.content.ContextCompat.getColor(b.root.context, com.pigmenta.app.R.color.bg_elevated)
                else
                    androidx.core.content.ContextCompat.getColor(b.root.context, com.pigmenta.app.R.color.bg_surface)
            )
            b.root.setOnClickListener {
                val prev = selectedIndex
                selectedIndex = position
                notifyItemChanged(prev)
                notifyItemChanged(selectedIndex)
                onSelect(preset)
            }
        }

        private fun Int.dpToPx(ctx: android.content.Context): Int =
            (this * ctx.resources.displayMetrics.density).toInt()
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder =
        ViewHolder(
            ItemFilterPresetBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        )

    override fun onBindViewHolder(holder: ViewHolder, position: Int) =
        holder.bind(presets[position], position == selectedIndex, position)

    override fun getItemCount() = presets.size
}
