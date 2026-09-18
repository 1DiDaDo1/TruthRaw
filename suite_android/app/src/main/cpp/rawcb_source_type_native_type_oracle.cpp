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

SetterTest finishTest(ACaptureRequest* request, uint32_t tag, camera_status_t createStatus,
                      camera_status_t setStatus, uint8_t expectedType) {
    SetterTest out;
    out.createStatus = createStatus;
    out.setStatus = setStatus;
    if (request != nullptr && setStatus == ACAMERA_OK) {
        ACameraMetadata_const_entry entry{};
        out.getStatus = ACaptureRequest_getConstEntry(request, tag, &entry);
        if (out.getStatus == ACAMERA_OK) {
            out.entryType = static_cast<int>(entry.type);
            out.entryCount = entry.count;
            out.accepted = entry.type == expectedType && entry.count == 1;
        }
    }
    if (request != nullptr) ACaptureRequest_free(request);
    return out;
}

SetterTest testU8(ACameraDevice* device, uint32_t tag) {
    ACaptureRequest* request = nullptr;
    const camera_status_t createStatus = ACameraDevice_createCaptureRequest(device, TEMPLATE_STILL_CAPTURE, &request);
    if (createStatus != ACAMERA_OK || request == nullptr) return finishTest(request, tag, createStatus, ACAMERA_ERROR_UNKNOWN, ACAMERA_TYPE_BYTE);
    const uint8_t one = 1;
    return finishTest(request, tag, createStatus, ACaptureRequest_setEntry_u8(request, tag, 1, &one), ACAMERA_TYPE_BYTE);
}

SetterTest testI32(ACameraDevice* device, uint32_t tag) {
    ACaptureRequest* request = nullptr;
    const camera_status_t createStatus = ACameraDevice_createCaptureRequest(device, TEMPLATE_STILL_CAPTURE, &request);
    if (createStatus != ACAMERA_OK || request == nullptr) return finishTest(request, tag, createStatus, ACAMERA_ERROR_UNKNOWN, ACAMERA_TYPE_INT32);
    const int32_t one = 1;
    return finishTest(request, tag, createStatus, ACaptureRequest_setEntry_i32(request, tag, 1, &one), ACAMERA_TYPE_INT32);
}

SetterTest testFloat(ACameraDevice* device, uint32_t tag) {
    ACaptureRequest* request = nullptr;
    const camera_status_t createStatus = ACameraDevice_createCaptureRequest(device, TEMPLATE_STILL_CAPTURE, &request);
    if (createStatus != ACAMERA_OK || request == nullptr) return finishTest(request, tag, createStatus, ACAMERA_ERROR_UNKNOWN, ACAMERA_TYPE_FLOAT);
    const float one = 1.0f;
    return finishTest(request, tag, createStatus, ACaptureRequest_setEntry_float(request, tag, 1, &one), ACAMERA_TYPE_FLOAT);
}

SetterTest testI64(ACameraDevice* device, uint32_t tag) {
    ACaptureRequest* request = nullptr;
    const camera_status_t createStatus = ACameraDevice_createCaptureRequest(device, TEMPLATE_STILL_CAPTURE, &request);
    if (createStatus != ACAMERA_OK || request == nullptr) return finishTest(request, tag, createStatus, ACAMERA_ERROR_UNKNOWN, ACAMERA_TYPE_INT64);
    const int64_t one = 1;
    return finishTest(request, tag, createStatus, ACaptureRequest_setEntry_i64(request, tag, 1, &one), ACAMERA_TYPE_INT64);
}

SetterTest testDouble(ACameraDevice* device, uint32_t tag) {
    ACaptureRequest* request = nullptr;
    const camera_status_t createStatus = ACameraDevice_createCaptureRequest(device, TEMPLATE_STILL_CAPTURE, &request);
    if (createStatus != ACAMERA_OK || request == nullptr) return finishTest(request, tag, createStatus, ACAMERA_ERROR_UNKNOWN, ACAMERA_TYPE_DOUBLE);
    const double one = 1.0;
    return finishTest(request, tag, createStatus, ACaptureRequest_setEntry_double(request, tag, 1, &one), ACAMERA_TYPE_DOUBLE);
}

SetterTest testRational(ACameraDevice* device, uint32_t tag) {
    ACaptureRequest* request = nullptr;
    const camera_status_t createStatus = ACameraDevice_createCaptureRequest(device, TEMPLATE_STILL_CAPTURE, &request);
    if (createStatus != ACAMERA_OK || request == nullptr) return finishTest(request, tag, createStatus, ACAMERA_ERROR_UNKNOWN, ACAMERA_TYPE_RATIONAL);
    const ACameraMetadata_rational one{1, 1};
    return finishTest(request, tag, createStatus, ACaptureRequest_setEntry_rational(request, tag, 1, &one), ACAMERA_TYPE_RATIONAL);
}

void appendTest(std::ostringstream& out, const char* name, const SetterTest& t) {
    out << "\"" << name << "\":{";
    out << "\"createStatus\":" << t.createStatus << ',';
    out << "\"setStatus\":" << t.setStatus << ',';
    out << "\"getStatus\":" << t.getStatus << ',';
    out << "\"accepted\":" << (t.accepted ? "true" : "false") << ',';
    out << "\"entryType\":" << t.entryType << ',';
    out << "\"entryCount\":" << t.entryCount;
    out << '}';
}

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_Camera2RawCbSourceTypeNativeTypeOracle_nativeProbe(
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

    camera_status_t characteristicsStatus = ACAMERA_ERROR_UNKNOWN;
    camera_status_t tagLookupStatus = ACAMERA_ERROR_UNKNOWN;
    camera_status_t openStatus = ACAMERA_ERROR_UNKNOWN;
    uint32_t tag = 0;
    bool symbolAvailable = false;
    SetterTest u8{};
    SetterTest i32{};
    SetterTest f32{};
    SetterTest i64{};
    SetterTest f64{};
    SetterTest rational{};
    std::string classification = "RAWCB_NATIVE_TYPE_ORACLE_UNINITIALIZED";
    std::string resolved = "UNRESOLVED";
    std::string error;

    if (manager == nullptr) {
        classification = "RAWCB_NATIVE_TYPE_ORACLE_MANAGER_CREATE_FAILED";
        error = "ACameraManager_create returned null";
    } else {
        characteristicsStatus = ACameraManager_getCameraCharacteristics(manager, camera_id.c_str(), &chars);
        if (characteristicsStatus != ACAMERA_OK || chars == nullptr) {
            classification = "RAWCB_NATIVE_TYPE_ORACLE_CHARACTERISTICS_FAILED";
        } else {
            camera2ndk = dlopen("libcamera2ndk.so", RTLD_NOW | RTLD_LOCAL);
            auto* fn = camera2ndk == nullptr
                ? nullptr
                : reinterpret_cast<GetTagFromNameFn>(dlsym(camera2ndk, "ACameraMetadata_getTagFromName"));
            symbolAvailable = fn != nullptr;
            if (fn == nullptr) {
                classification = "RAWCB_NATIVE_TYPE_ORACLE_TAG_LOOKUP_SYMBOL_UNAVAILABLE";
                const char* dlerr = dlerror();
                error = dlerr == nullptr ? "ACameraMetadata_getTagFromName unavailable" : dlerr;
            } else {
                tagLookupStatus = fn(chars, key_name.c_str(), &tag);
                if (tagLookupStatus != ACAMERA_OK) {
                    classification = "RAWCB_NATIVE_TYPE_ORACLE_VENDOR_TAG_LOOKUP_FAILED";
                } else {
                    ACameraDevice_StateCallbacks callbacks{};
                    callbacks.context = nullptr;
                    callbacks.onDisconnected = onDisconnected;
                    callbacks.onError = onError;
                    openStatus = ACameraManager_openCamera(manager, camera_id.c_str(), &callbacks, &device);
                    if (openStatus != ACAMERA_OK || device == nullptr) {
                        classification = "RAWCB_NATIVE_TYPE_ORACLE_CAMERA_OPEN_FAILED";
                    } else {
                        u8 = testU8(device, tag);
                        i32 = testI32(device, tag);
                        f32 = testFloat(device, tag);
                        i64 = testI64(device, tag);
                        f64 = testDouble(device, tag);
                        rational = testRational(device, tag);

                        int acceptedCount = 0;
                        if (u8.accepted) { ++acceptedCount; resolved = "BYTE"; }
                        if (i32.accepted) { ++acceptedCount; resolved = "INT32"; }
                        if (f32.accepted) { ++acceptedCount; resolved = "FLOAT"; }
                        if (i64.accepted) { ++acceptedCount; resolved = "INT64"; }
                        if (f64.accepted) { ++acceptedCount; resolved = "DOUBLE"; }
                        if (rational.accepted) { ++acceptedCount; resolved = "RATIONAL"; }

                        if (acceptedCount == 1) {
                            classification = "NATIVE_METADATA_TYPE_" + resolved + "__NO_SESSION_OR_CAPTURE_SUBMISSION";
                        } else if (acceptedCount == 0) {
                            classification = "RAWCB_NATIVE_TYPE_ORACLE_UNRESOLVED_NONE_ACCEPTED";
                            resolved = "UNRESOLVED";
                        } else {
                            classification = "RAWCB_NATIVE_TYPE_ORACLE_AMBIGUOUS_MULTIPLE_ACCEPTED";
                            resolved = "AMBIGUOUS";
                        }
                    }
                }
            }
        }
    }

    if (device != nullptr) ACameraDevice_close(device);
    if (chars != nullptr) ACameraMetadata_free(chars);
    if (manager != nullptr) ACameraManager_delete(manager);
    if (camera2ndk != nullptr) dlclose(camera2ndk);

    const int acceptedCount = (u8.accepted ? 1 : 0) + (i32.accepted ? 1 : 0) +
        (f32.accepted ? 1 : 0) + (i64.accepted ? 1 : 0) +
        (f64.accepted ? 1 : 0) + (rational.accepted ? 1 : 0);

    std::ostringstream out;
    out << '{';
    out << "\"schema\":\"truthraw.camera2-rawcb-source-type-native-type-oracle.v0.25\",";
    out << "\"cameraId\":\"" << escapeJson(camera_id) << "\",";
    out << "\"keyName\":\"" << escapeJson(key_name) << "\",";
    out << "\"classification\":\"" << classification << "\",";
    out << "\"characteristicsStatus\":" << characteristicsStatus << ',';
    out << "\"tagLookupSymbolAvailable\":" << (symbolAvailable ? "true" : "false") << ',';
    out << "\"tagLookupStatus\":" << tagLookupStatus << ',';
    out << "\"tagIdUnsigned\":" << static_cast<unsigned long long>(tag) << ',';
    out << "\"tagIdHex\":\"0x" << std::hex << std::uppercase << tag << std::dec << "\",";
    out << "\"cameraOpenStatus\":" << openStatus << ',';
    appendTest(out, "u8Test", u8); out << ',';
    appendTest(out, "i32Test", i32); out << ',';
    appendTest(out, "floatTest", f32); out << ',';
    appendTest(out, "i64Test", i64); out << ',';
    appendTest(out, "doubleTest", f64); out << ',';
    appendTest(out, "rationalTest", rational); out << ',';
    out << "\"acceptedTypeCount\":" << acceptedCount << ',';
    out << "\"nativeMetadataTypeResolved\":" << (acceptedCount == 1 ? "true" : "false") << ',';
    out << "\"resolvedNativeType\":\"" << resolved << "\",";
    out << "\"testValuePurpose\":\"TYPE_VALIDATION_ONLY_NOT_VENDOR_VALUE_SEMANTICS\",";
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


extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_Camera2RawCbSourceTypeNativeTypeOracle_nativeProbeDecoupled(
    JNIEnv* env,
    jobject,
    jstring metadata_camera_id_j,
    jstring request_camera_id_j,
    jstring key_name_j) {
    const char* metadata_camera_id_chars = env->GetStringUTFChars(metadata_camera_id_j, nullptr);
    const char* request_camera_id_chars = env->GetStringUTFChars(request_camera_id_j, nullptr);
    const char* key_name_chars = env->GetStringUTFChars(key_name_j, nullptr);
    const std::string metadata_camera_id =
        metadata_camera_id_chars != nullptr ? metadata_camera_id_chars : "";
    const std::string request_camera_id =
        request_camera_id_chars != nullptr ? request_camera_id_chars : "";
    const std::string key_name = key_name_chars != nullptr ? key_name_chars : "";
    if (metadata_camera_id_chars != nullptr) {
        env->ReleaseStringUTFChars(metadata_camera_id_j, metadata_camera_id_chars);
    }
    if (request_camera_id_chars != nullptr) {
        env->ReleaseStringUTFChars(request_camera_id_j, request_camera_id_chars);
    }
    if (key_name_chars != nullptr) env->ReleaseStringUTFChars(key_name_j, key_name_chars);

    ACameraManager* manager = ACameraManager_create();
    ACameraMetadata* metadata_chars = nullptr;
    ACameraDevice* request_device = nullptr;
    void* camera2ndk = nullptr;

    camera_status_t characteristicsStatus = ACAMERA_ERROR_UNKNOWN;
    camera_status_t tagLookupStatus = ACAMERA_ERROR_UNKNOWN;
    camera_status_t requestCameraOpenStatus = ACAMERA_ERROR_UNKNOWN;
    uint32_t tag = 0;
    bool symbolAvailable = false;
    SetterTest u8{};
    SetterTest i32{};
    SetterTest f32{};
    SetterTest i64{};
    SetterTest f64{};
    SetterTest rational{};
    std::string classification = "DECOUPLED_NATIVE_TYPE_ORACLE_UNINITIALIZED";
    std::string resolved = "UNRESOLVED";
    std::string error;

    if (manager == nullptr) {
        classification = "DECOUPLED_NATIVE_TYPE_ORACLE_MANAGER_CREATE_FAILED";
        error = "ACameraManager_create returned null";
    } else {
        characteristicsStatus = ACameraManager_getCameraCharacteristics(
            manager, metadata_camera_id.c_str(), &metadata_chars);
        if (characteristicsStatus != ACAMERA_OK || metadata_chars == nullptr) {
            classification = "DECOUPLED_NATIVE_TYPE_ORACLE_METADATA_CHARACTERISTICS_FAILED";
        } else {
            camera2ndk = dlopen("libcamera2ndk.so", RTLD_NOW | RTLD_LOCAL);
            auto* fn = camera2ndk == nullptr
                ? nullptr
                : reinterpret_cast<GetTagFromNameFn>(
                    dlsym(camera2ndk, "ACameraMetadata_getTagFromName"));
            symbolAvailable = fn != nullptr;
            if (fn == nullptr) {
                classification = "DECOUPLED_NATIVE_TYPE_ORACLE_TAG_LOOKUP_SYMBOL_UNAVAILABLE";
                const char* dlerr = dlerror();
                error = dlerr == nullptr
                    ? "ACameraMetadata_getTagFromName unavailable"
                    : dlerr;
            } else {
                tagLookupStatus = fn(metadata_chars, key_name.c_str(), &tag);
                if (tagLookupStatus != ACAMERA_OK) {
                    classification = "DECOUPLED_NATIVE_TYPE_ORACLE_VENDOR_TAG_LOOKUP_FAILED";
                } else {
                    ACameraDevice_StateCallbacks callbacks{};
                    callbacks.context = nullptr;
                    callbacks.onDisconnected = onDisconnected;
                    callbacks.onError = onError;
                    requestCameraOpenStatus = ACameraManager_openCamera(
                        manager, request_camera_id.c_str(), &callbacks, &request_device);
                    if (requestCameraOpenStatus != ACAMERA_OK || request_device == nullptr) {
                        classification = "DECOUPLED_NATIVE_TYPE_ORACLE_REQUEST_CAMERA_OPEN_FAILED";
                    } else {
                        u8 = testU8(request_device, tag);
                        i32 = testI32(request_device, tag);
                        f32 = testFloat(request_device, tag);
                        i64 = testI64(request_device, tag);
                        f64 = testDouble(request_device, tag);
                        rational = testRational(request_device, tag);

                        int acceptedCount = 0;
                        if (u8.accepted) { ++acceptedCount; resolved = "BYTE"; }
                        if (i32.accepted) { ++acceptedCount; resolved = "INT32"; }
                        if (f32.accepted) { ++acceptedCount; resolved = "FLOAT"; }
                        if (i64.accepted) { ++acceptedCount; resolved = "INT64"; }
                        if (f64.accepted) { ++acceptedCount; resolved = "DOUBLE"; }
                        if (rational.accepted) { ++acceptedCount; resolved = "RATIONAL"; }

                        if (acceptedCount == 1) {
                            classification =
                                "DECOUPLED_NATIVE_METADATA_TYPE_" + resolved +
                                "__PHYSICAL_TAG_LOOKUP_LOGICAL_REQUEST_TEMPLATE__NO_SUBMIT";
                        } else if (acceptedCount == 0) {
                            classification =
                                "DECOUPLED_NATIVE_TYPE_ORACLE_UNRESOLVED_NONE_ACCEPTED";
                            resolved = "UNRESOLVED";
                        } else {
                            classification =
                                "DECOUPLED_NATIVE_TYPE_ORACLE_AMBIGUOUS_MULTIPLE_ACCEPTED";
                            resolved = "AMBIGUOUS";
                        }
                    }
                }
            }
        }
    }

    if (request_device != nullptr) ACameraDevice_close(request_device);
    if (metadata_chars != nullptr) ACameraMetadata_free(metadata_chars);
    if (manager != nullptr) ACameraManager_delete(manager);
    if (camera2ndk != nullptr) dlclose(camera2ndk);

    const int acceptedCount = (u8.accepted ? 1 : 0) + (i32.accepted ? 1 : 0) +
        (f32.accepted ? 1 : 0) + (i64.accepted ? 1 : 0) +
        (f64.accepted ? 1 : 0) + (rational.accepted ? 1 : 0);

    std::ostringstream out;
    out << '{';
    out << "\"schema\":\"truthraw.camera2-decoupled-native-type-oracle.v0.36\",";
    out << "\"metadataCameraId\":\"" << escapeJson(metadata_camera_id) << "\",";
    out << "\"requestCameraId\":\"" << escapeJson(request_camera_id) << "\",";
    out << "\"keyName\":\"" << escapeJson(key_name) << "\",";
    out << "\"classification\":\"" << classification << "\",";
    out << "\"characteristicsStatus\":" << characteristicsStatus << ',';
    out << "\"tagLookupSymbolAvailable\":" << (symbolAvailable ? "true" : "false") << ',';
    out << "\"tagLookupStatus\":" << tagLookupStatus << ',';
    out << "\"tagIdUnsigned\":" << static_cast<unsigned long long>(tag) << ',';
    out << "\"tagIdHex\":\"0x" << std::hex << std::uppercase << tag << std::dec << "\",";
    out << "\"requestCameraOpenStatus\":" << requestCameraOpenStatus << ',';
    appendTest(out, "u8Test", u8); out << ',';
    appendTest(out, "i32Test", i32); out << ',';
    appendTest(out, "floatTest", f32); out << ',';
    appendTest(out, "i64Test", i64); out << ',';
    appendTest(out, "doubleTest", f64); out << ',';
    appendTest(out, "rationalTest", rational); out << ',';
    out << "\"acceptedTypeCount\":" << acceptedCount << ',';
    out << "\"nativeMetadataTypeResolved\":" << (acceptedCount == 1 ? "true" : "false") << ',';
    out << "\"resolvedNativeType\":\"" << resolved << "\",";
    out << "\"lookupAuthority\":\"PHYSICAL_CAMERA_CHARACTERISTICS_ONLY\",";
    out << "\"requestTemplateAuthority\":\"LOGICAL_CAMERA_DISPOSABLE_TEMPLATE_ONLY\",";
    out << "\"directPhysicalCameraOpenAttempted\":false,";
    out << "\"testValuePurpose\":\"TYPE_VALIDATION_ONLY_NOT_VENDOR_VALUE_SEMANTICS\",";
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
