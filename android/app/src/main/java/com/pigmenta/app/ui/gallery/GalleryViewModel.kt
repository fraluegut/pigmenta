package com.pigmenta.app.ui.gallery

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.pigmenta.app.data.local.ImageEntity
import com.pigmenta.app.data.repository.ImageRepository
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch
import javax.inject.Inject

@HiltViewModel
class GalleryViewModel @Inject constructor(
    private val repository: ImageRepository,
) : ViewModel() {

    val images: StateFlow<List<ImageEntity>> = repository.savedImages
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5_000), emptyList())

    fun deleteImage(entity: ImageEntity) {
        viewModelScope.launch { repository.deleteImage(entity) }
    }
}
