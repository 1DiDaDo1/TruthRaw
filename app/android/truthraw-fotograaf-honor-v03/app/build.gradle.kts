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

    kotlinOptions {
        jvmTarget = "17"
    }

    defaultConfig {
        applicationId = "com.truthraw.adaptiveui"
        minSdk = 31
        targetSdk = 35
        versionCode = 7
        versionName = "0.7-fotograaf-permission-gate"

        ndk {
            abiFilters += listOf("arm64-v8a")
        }

        externalNativeBuild {
            cmake {
                cppFlags += listOf("-std=c++20", "-Wall", "-Wextra", "-Werror")
            }
        }
    }

    sourceSets {
        getByName("main") {
            java.srcDir("../../truthraw-adaptive-ui-v01/app/src/main/java")
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../truthraw-adaptive-ui-v01/app/src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}
