package com.pigmenta.app.data.remote.dto

import com.google.gson.annotations.SerializedName

data class FilterListResponse(
    @SerializedName("filters") val filters: List<FilterDto>
)

data class FilterDto(
    @SerializedName("id")          val id: Int,
    @SerializedName("name")        val name: String,
    @SerializedName("description") val description: String,
)

data class PigmentAnalysisResponse(
    @SerializedName("n_clusters")          val nClusters: Int,
    @SerializedName("dominant_colors")     val dominantColors: List<List<Int>>,
    @SerializedName("color_names")         val colorNames: List<String>,
    @SerializedName("pigment_confidence")  val pigmentConfidence: Float,
    @SerializedName("notes")              val notes: String,
)
