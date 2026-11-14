package com.pigmenta.app.data.remote

import com.pigmenta.app.data.remote.dto.FilterListResponse
import com.pigmenta.app.data.remote.dto.PigmentAnalysisResponse
import okhttp3.MultipartBody
import okhttp3.RequestBody
import okhttp3.ResponseBody
import retrofit2.Response
import retrofit2.http.*

interface PigmentaApiService {

    @GET("health")
    suspend fun health(): Response<Map<String, String>>

    @GET("images/filters")
    suspend fun listFilters(): FilterListResponse

    @Multipart
    @POST("images/process")
    suspend fun processImage(
        @Part file: MultipartBody.Part,
        @Part("filter_type") filterType: RequestBody,
        @Part("intensity") intensity: RequestBody,
        @Part("target_std") targetStd: RequestBody,
    ): Response<ResponseBody>

    @Multipart
    @POST("images/analyse")
    suspend fun analyseImage(
        @Part file: MultipartBody.Part,
        @Part("n_clusters") nClusters: RequestBody,
        @Part("filter_type") filterType: RequestBody,
        @Part("intensity") intensity: RequestBody,
    ): PigmentAnalysisResponse
}
