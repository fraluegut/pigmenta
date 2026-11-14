package com.pigmenta.app.data.di

import android.content.Context
import androidx.room.Room
import com.pigmenta.app.data.local.ImageDao
import com.pigmenta.app.data.local.PigmentaDatabase
import com.pigmenta.app.data.remote.PigmentaApiService
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.qualifiers.ApplicationContext
import dagger.hilt.components.SingletonComponent
import okhttp3.OkHttpClient
import okhttp3.logging.HttpLoggingInterceptor
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory
import java.util.concurrent.TimeUnit
import javax.inject.Singleton

@Module
@InstallIn(SingletonComponent::class)
object AppModule {

    // ── Room ──────────────────────────────────────────────────────────────────

    @Provides
    @Singleton
    fun provideDatabase(@ApplicationContext ctx: Context): PigmentaDatabase =
        Room.databaseBuilder(ctx, PigmentaDatabase::class.java, "pigmenta.db")
            .fallbackToDestructiveMigration()
            .build()

    @Provides
    fun provideImageDao(db: PigmentaDatabase): ImageDao = db.imageDao()

    // ── Retrofit ──────────────────────────────────────────────────────────────

    @Provides
    @Singleton
    fun provideOkHttp(): OkHttpClient = OkHttpClient.Builder()
        .connectTimeout(30, TimeUnit.SECONDS)
        .readTimeout(60, TimeUnit.SECONDS)
        .addInterceptor(HttpLoggingInterceptor().apply {
            level = HttpLoggingInterceptor.Level.BASIC
        })
        .build()

    @Provides
    @Singleton
    fun provideApiService(client: OkHttpClient): PigmentaApiService =
        Retrofit.Builder()
            // Override in production via BuildConfig or remote config
            .baseUrl("http://10.0.2.2:8000/")  // localhost from Android emulator
            .client(client)
            .addConverterFactory(GsonConverterFactory.create())
            .build()
            .create(PigmentaApiService::class.java)
}
