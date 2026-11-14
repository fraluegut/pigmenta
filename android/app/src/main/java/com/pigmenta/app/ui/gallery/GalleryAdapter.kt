package com.pigmenta.app.ui.gallery
import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import coil.load
import com.pigmenta.app.data.local.ImageEntity
import com.pigmenta.app.databinding.ItemGalleryBinding
import java.io.File
import java.text.SimpleDateFormat
import java.util.*
class GalleryAdapter(
    private val onClick: (ImageEntity) -> Unit,
    private val onDelete: (ImageEntity) -> Unit,
) : ListAdapter<ImageEntity, GalleryAdapter.ViewHolder>(DIFF) {
    inner class ViewHolder(private val b: ItemGalleryBinding) : RecyclerView.ViewHolder(b.root) {
        fun bind(entity: ImageEntity) {
            b.imageView.load(File(entity.processedPath)) { crossfade(true) }
            b.tvFilterName.text = entity.filterName
            val dateStr = SimpleDateFormat("dd/MM/yy  HH:mm", Locale.getDefault()).format(Date(entity.createdAt))
            b.tvDate.text = if (entity.siteLabel.isNotEmpty()) "${entity.siteLabel} * $dateStr" else dateStr
            b.root.setOnClickListener { onClick(entity) }
            b.root.setOnLongClickListener {
                com.google.android.material.dialog.MaterialAlertDialogBuilder(b.root.context)
                    .setTitle("Eliminar imagen?")
                    .setMessage("Se eliminara de la aplicacion (no de la galeria del movil).")
                    .setPositiveButton("Eliminar") { _, _ -> onDelete(entity) }
                    .setNegativeButton("Cancelar", null)
                    .show()
                true
            }
        }
    }
    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder =
        ViewHolder(ItemGalleryBinding.inflate(LayoutInflater.from(parent.context), parent, false))
    override fun onBindViewHolder(holder: ViewHolder, position: Int) = holder.bind(getItem(position))
    companion object {
        private val DIFF = object : DiffUtil.ItemCallback<ImageEntity>() {
            override fun areItemsTheSame(a: ImageEntity, b: ImageEntity) = a.id == b.id
            override fun areContentsTheSame(a: ImageEntity, b: ImageEntity) = a == b
        }
    }
}
