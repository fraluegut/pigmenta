package com.pigmenta.app.data.local

import androidx.room.Database
import androidx.room.RoomDatabase

@Database(entities = [ImageEntity::class], version = 2, exportSchema = false)
abstract class PigmentaDatabase : RoomDatabase() {
    abstract fun imageDao(): ImageDao
}
