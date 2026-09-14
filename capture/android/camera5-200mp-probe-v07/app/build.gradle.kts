plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.truthraw.fullsensorprobe"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.truthraw.fullsensorprobe.v04"
        minSdk = 31
        targetSdk = 35
        versionCode = 8
        versionName = "0.4-sample-domain"
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.15.0")
}
