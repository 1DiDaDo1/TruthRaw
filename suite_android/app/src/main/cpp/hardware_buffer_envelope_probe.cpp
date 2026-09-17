#include <jni.h>
#include <android/hardware_buffer.h>
#include <android/hardware_buffer_jni.h>

#include <cstdint>
#include <sstream>

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_Camera2EnvelopeProbe_nativeDescribeHardwareBuffer(
    JNIEnv* env,
    jobject,
    jobject hardware_buffer_object) {
    if (hardware_buffer_object == nullptr) {
        return env->NewStringUTF("{\"available\":false,\"error\":\"null HardwareBuffer\"}");
    }

    AHardwareBuffer* buffer = AHardwareBuffer_fromHardwareBuffer(env, hardware_buffer_object);
    if (buffer == nullptr) {
        return env->NewStringUTF("{\"available\":false,\"error\":\"AHardwareBuffer_fromHardwareBuffer returned null\"}");
    }

    AHardwareBuffer_Desc desc{};
    AHardwareBuffer_describe(buffer, &desc);

    std::uint64_t buffer_id = 0;
    const int id_status = AHardwareBuffer_getId(buffer, &buffer_id);

    std::ostringstream out;
    out << "{"
        << "\"available\":true,"
        << "\"width\":" << desc.width << ","
        << "\"height\":" << desc.height << ","
        << "\"layers\":" << desc.layers << ","
        << "\"format\":" << desc.format << ","
        << "\"usage\":" << desc.usage << ","
        << "\"stridePixels\":" << desc.stride << ","
        << "\"getIdStatus\":" << id_status << ","
        << "\"bufferId\":" << buffer_id << ","
        << "\"lockedByProbe\":false,"
        << "\"mappedByProbe\":false,"
        << "\"writtenByProbe\":false"
        << "}";

    const std::string json = out.str();
    return env->NewStringUTF(json.c_str());
}
