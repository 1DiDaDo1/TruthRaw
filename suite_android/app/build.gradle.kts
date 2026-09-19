plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.truthraw.adaptiveui"
    compileSdk = 35
    ndkVersion = "27.2.12479018"

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions { jvmTarget = "17" }

    defaultConfig {
        applicationId = "com.truthraw.adaptiveui"
        minSdk = 31
        targetSdk = 37
        versionCode = 22
        versionName = "0.22-v0.57-pure-float32-output"

        ndk { abiFilters += listOf("arm64-v8a") }
        externalNativeBuild { cmake { cppFlags += listOf("-std=c++20", "-Wall", "-Wextra", "-Werror") } }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}