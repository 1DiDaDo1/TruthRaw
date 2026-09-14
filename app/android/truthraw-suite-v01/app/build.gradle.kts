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
        applicationId = "com.truthraw.suite"
        minSdk = 31
        targetSdk = 35
        versionCode = 1
        versionName = "0.1-main-plus-fotograaf"

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
            java.srcDirs(
                "src/main/java",
                "../../truthraw-adaptive-ui-v01/app/src/main/java",
                "../../truthraw-fotograaf-capture-v01/app/src/main/java"
            )
            res.srcDirs("../../truthraw-adaptive-ui-v01/app/src/main/res")
            manifest.srcFile("src/main/AndroidManifest.xml")
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../truthraw-adaptive-ui-v01/app/src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}
