package com.pigmenta.app.data.local

import androidx.room.*
import kotlinx.coroutines.flow.Flow

@Dao
interface ImageDao {

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insert(image: ImageEntity): Long

    @Delete
    suspend fun delete(image: ImageEntity)

    @Query("SELECT * FROM processed_images ORDER BY createdAt DESC")
    fun observeAll(): Flow<List<ImageEntity>>

    @Query("SELECT * FROM processed_images WHERE id = :id")
    suspend fun findById(id: Long): ImageEntity?

    @Query("DELETE FROM processed_images WHERE id = :id")
    suspend fun deleteById(id: Long)
}
