#include <jni.h>

#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadata.h>
#include <camera/NdkCaptureRequest.h>

#include <dlfcn.h>

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace {

using GetTagFromNameFn = camera_status_t (*)(const ACameraMetadata*, const char*, uint32_t*);

struct SetterTest {
    camera_status_t createStatus = ACAMERA_ERROR_UNKNOWN;
    camera_status_t setStatus = ACAMERA_ERROR_UNKNOWN;
    camera_status_t getStatus = ACAMERA_ERROR_UNKNOWN;
    bool accepted = false;
    int entryType = -1;
    uint32_t entryCount = 0;
    long long firstValue = 0;
};

void onDisconnected(void*, ACameraDevice*) {}
void onError(void*, ACameraDevice*, int) {}

std::string escapeJson(const std::string& in) {
    std::ostringstream out;
    for (const char c : in) {
        switch (c) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << c; break;
        }
    }
    return out.str();
}

SetterTest testU8(ACameraDevice* device, uint32_t tag) {
    SetterTest out;
    ACaptureRequest* request = nullptr;
    out.createStatus = ACameraDevice_createCaptureRequest(device, TEMPLATE_STILL_CAPTURE, &request);
    if (out.createStatus != ACAMERA_OK || request == nullptr) return out;

    const uint8_t one = 1;
    out.setStatus = ACaptureRequest_setEntry_u8(request, tag, 1, &one);
    if (out.setStatus == ACAMERA_OK) {
        ACameraMetadata_const_entry entry{};
        out.getStatus = ACaptureRequest_getConstEntry(request, tag, &entry);
        if (out.getStatus == ACAMERA_OK) {
            out.entryType = static_cast<int>(entry.type);
            out.entryCount = entry.count;
            if (entry.count > 0 && entry.data.u8 != nullptr) {
                out.firstValue = entry.data.u8[0];
            }
            out.accepted = entry.type == ACAMERA_TYPE_BYTE && entry.count == 1 && out.firstValue == 1;
        }
    }
    ACaptureRequest_free(request);
    return out;
}

SetterTest testI32(ACameraDevice* device, uint32_t tag) {
    SetterTest out;
    ACaptureRequest* request = nullptr;
    out.createStatus = ACameraDevice_createCaptureRequest(device, TEMPLATE_STILL_CAPTURE, &request);
    if (out.createStatus != ACAMERA_OK || request == nullptr) return out;

    const int32_t one = 1;
    out.setStatus = ACaptureRequest_setEntry_i32(request, tag, 1, &one);
    if (out.setStatus == ACAMERA_OK) {
        ACameraMetadata_const_entry entry{};
        out.getStatus = ACaptureRequest_getConstEntry(request, tag, &entry);
        if (out.getStatus == ACAMERA_OK) {
            out.entryType = static_cast<int>(entry.type);
            out.entryCount = entry.count;
            if (entry.count > 0 && entry.data.i32 != nullptr) {
                out.firstValue = entry.data.i32[0];
            }
            out.accepted = entry.type == ACAMERA_TYPE_INT32 && entry.count == 1 && out.firstValue == 1;
        }
    }
    ACaptureRequest_free(request);
    return out;
}

void appendTest(std::ostringstream& out, const char* name, const SetterTest& t) {
    out << "\"" << name << "\":{";
    out << "\"createStatus\":" << t.createStatus << ',';
    out << "\"setStatus\":" << t.setStatus << ',';
    out << "\"getStatus\":" << t.getStatus << ',';
    out << "\"accepted\":" << (t.accepted ? "true" : "false") << ',';
    out << "\"entryType\":" << t.entryType << ',';
    out << "\"entryCount\":" << t.entryCount << ',';
    out << "\"firstValue\":" << t.firstValue;
    out << '}';
}

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_Camera2IdealRawNativeTypeOracle_nativeProbe(
    JNIEnv* env,
    jobject,
    jstring camera_id_j,
    jstring key_name_j) {
    const char* camera_id_chars = env->GetStringUTFChars(camera_id_j, nullptr);
    const char* key_name_chars = env->GetStringUTFChars(key_name_j, nullptr);
    const std::string camera_id = camera_id_chars != nullptr ? camera_id_chars : "";
    const std::string key_name = key_name_chars != nullptr ? key_name_chars : "";
    if (camera_id_chars != nullptr) env->ReleaseStringUTFChars(camera_id_j, camera_id_chars);
    if (key_name_chars != nullptr) env->ReleaseStringUTFChars(key_name_j, key_name_chars);

    ACameraManager* manager = ACameraManager_create();
    ACameraMetadata* chars = nullptr;
    ACameraDevice* device = nullptr;
    void* camera2ndk = nullptr;

    camera_status_t characteristics_status = ACAMERA_ERROR_UNKNOWN;
    camera_status_t tag_lookup_status = ACAMERA_ERROR_UNKNOWN;
    camera_status_t open_status = ACAMERA_ERROR_UNKNOWN;
    uint32_t tag = 0;
    bool symbol_available = false;
    SetterTest u8{};
    SetterTest i32{};
    std::string classification = "NATIVE_TYPE_ORACLE_UNINITIALIZED";
    std::string error;

    if (manager == nullptr) {
        classification = "NATIVE_TYPE_ORACLE_MANAGER_CREATE_FAILED";
        error = "ACameraManager_create returned null";
    } else {
        characteristics_status = ACameraManager_getCameraCharacteristics(manager, camera_id.c_str(), &chars);
        if (characteristics_status != ACAMERA_OK || chars == nullptr) {
            classification = "NATIVE_TYPE_ORACLE_CHARACTERISTICS_FAILED";
        } else {
            camera2ndk = dlopen("libcamera2ndk.so", RTLD_NOW | RTLD_LOCAL);
            auto* fn = camera2ndk == nullptr
                ? nullptr
                : reinterpret_cast<GetTagFromNameFn>(dlsym(camera2ndk, "ACameraMetadata_getTagFromName"));
            symbol_available = fn != nullptr;
            if (fn == nullptr) {
                classification = "NATIVE_TYPE_ORACLE_TAG_LOOKUP_SYMBOL_UNAVAILABLE";
                error = dlerror() == nullptr ? "ACameraMetadata_getTagFromName unavailable" : dlerror();
            } else {
                tag_lookup_status = fn(chars, key_name.c_str(), &tag);
                if (tag_lookup_status != ACAMERA_OK) {
                    classification = "NATIVE_TYPE_ORACLE_VENDOR_TAG_LOOKUP_FAILED";
                } else {
                    ACameraDevice_StateCallbacks callbacks{};
                    callbacks.context = nullptr;
                    callbacks.onDisconnected = onDisconnected;
                    callbacks.onError = onError;
                    open_status = ACameraManager_openCamera(manager, camera_id.c_str(), &callbacks, &device);
                    if (open_status != ACAMERA_OK || device == nullptr) {
                        classification = "NATIVE_TYPE_ORACLE_CAMERA_OPEN_FAILED";
                    } else {
                        u8 = testU8(device, tag);
                        i32 = testI32(device, tag);
                        if (u8.accepted && !i32.accepted) {
                            classification = "NATIVE_METADATA_TYPE_BYTE__NO_SESSION_OR_CAPTURE_SUBMISSION";
                        } else if (i32.accepted && !u8.accepted) {
                            classification = "NATIVE_METADATA_TYPE_INT32__NO_SESSION_OR_CAPTURE_SUBMISSION";
                        } else if (u8.accepted && i32.accepted) {
                            classification = "NATIVE_TYPE_ORACLE_AMBIGUOUS_BOTH_ACCEPTED";
                        } else {
                            classification = "NATIVE_TYPE_ORACLE_UNRESOLVED_NEITHER_ACCEPTED";
                        }
                    }
                }
            }
        }
    }

    if (device != nullptr) {
        ACameraDevice_close(device);
        device = nullptr;
    }
    if (chars != nullptr) {
        ACameraMetadata_free(chars);
        chars = nullptr;
    }
    if (manager != nullptr) {
        ACameraManager_delete(manager);
        manager = nullptr;
    }
    if (camera2ndk != nullptr) {
        dlclose(camera2ndk);
        camera2ndk = nullptr;
    }

    std::ostringstream out;
    out << '{';
    out << "\"schema\":\"truthraw.camera2-idealraw-native-type-oracle.v0.1\",";
    out << "\"cameraId\":\"" << escapeJson(camera_id) << "\",";
    out << "\"keyName\":\"" << escapeJson(key_name) << "\",";
    out << "\"classification\":\"" << classification << "\",";
    out << "\"characteristicsStatus\":" << characteristics_status << ',';
    out << "\"tagLookupSymbolAvailable\":" << (symbol_available ? "true" : "false") << ',';
    out << "\"tagLookupStatus\":" << tag_lookup_status << ',';
    out << "\"tagIdUnsigned\":" << static_cast<unsigned long long>(tag) << ',';
    out << "\"tagIdHex\":\"0x" << std::hex << std::uppercase << tag << std::dec << "\",";
    out << "\"cameraOpenStatus\":" << open_status << ',';
    appendTest(out, "u8Test", u8);
    out << ',';
    appendTest(out, "i32Test", i32);
    out << ',';
    out << "\"nativeMetadataTypeResolved\":" << ((u8.accepted ^ i32.accepted) ? "true" : "false") << ',';
    out << "\"resolvedNativeType\":\""
        << (u8.accepted && !i32.accepted ? "BYTE" : (i32.accepted && !u8.accepted ? "INT32" : "UNRESOLVED"))
        << "\",";
    out << "\"requestTemplatesCreatedOnly\":true,";
    out << "\"sessionCreated\":false,";
    out << "\"sessionParametersAttached\":false,";
    out << "\"captureSubmitted\":false,";
    out << "\"vendorModifiedRequestSubmittedToHal\":false,";
    out << "\"rawPixelAccess\":false,";
    out << "\"sourceMutation\":false,";
    out << "\"semanticMeaningAssumed\":false,";
    out << "\"semanticPromotionAllowed\":false";
    if (!error.empty()) out << ",\"error\":\"" << escapeJson(error) << "\"";
    out << '}';

    const std::string json = out.str();
    return env->NewStringUTF(json.c_str());
}
