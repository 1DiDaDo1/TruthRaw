plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.truthraw.fullsensorprobe"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.truthraw.fullsensorprobe"
        minSdk = 31
        targetSdk = 35
        versionCode = 7
        versionName = "0.7"
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.15.0")
}
