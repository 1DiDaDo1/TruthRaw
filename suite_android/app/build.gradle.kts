plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

val drawSigningStoreFile = providers.environmentVariable("DRAW_SIGNING_STORE_FILE").orNull
val drawSigningStorePassword = providers.environmentVariable("DRAW_SIGNING_STORE_PASSWORD").orNull
val drawSigningKeyAlias = providers.environmentVariable("DRAW_SIGNING_KEY_ALIAS").orNull
val drawSigningKeyPassword = providers.environmentVariable("DRAW_SIGNING_KEY_PASSWORD").orNull
val hasDrawStableSigning =
    !drawSigningStoreFile.isNullOrBlank() &&
    !drawSigningStorePassword.isNullOrBlank() &&
    !drawSigningKeyAlias.isNullOrBlank() &&
    !drawSigningKeyPassword.isNullOrBlank()

android {
    namespace = "com.truthraw.adaptiveui"
    compileSdk = 35
    ndkVersion = "27.2.12479018"

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions { jvmTarget = "17" }

    signingConfigs {
        if (hasDrawStableSigning) {
            create("drawStableDebug") {
                storeFile = file(drawSigningStoreFile!!)
                storePassword = drawSigningStorePassword
                keyAlias = drawSigningKeyAlias
                keyPassword = drawSigningKeyPassword
            }
        }
    }

    defaultConfig {
        applicationId = "com.truthraw.adaptiveui"
        minSdk = 31
        targetSdk = 37
        versionCode = 51
        versionName = "0.51-v0.84.2-adaptive-compute-router"

        ndk { abiFilters += listOf("arm64-v8a") }
        externalNativeBuild { cmake { cppFlags += listOf("-std=c++20", "-Wall", "-Wextra", "-Werror") } }
    }

    buildTypes {
        getByName("debug") {
            if (hasDrawStableSigning) {
                signingConfig = signingConfigs.getByName("drawStableDebug")
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}