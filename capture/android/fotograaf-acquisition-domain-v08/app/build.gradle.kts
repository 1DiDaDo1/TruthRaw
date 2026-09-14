plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.truthraw.acquisitiondomainprobe"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.truthraw.acquisitiondomainprobe"
        minSdk = 31
        targetSdk = 35
        versionCode = 8
        versionName = "0.8"
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}

kotlin {
    jvmToolchain(17)
}

dependencies {
    implementation("androidx.core:core-ktx:1.15.0")
}
