#include <jni.h>

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_NativeTilePreviewBridge_exportFinalizedProjection(
    JNIEnv*, jobject, jint, jint, jint, jint, jint);

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_NativeRawProjectionBridge_exportFinalizedProjection(
    JNIEnv* env,
    jobject receiver,
    jint sourceFd,
    jint outputFd,
    jint projectionKind,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    return Java_com_truthraw_adaptiveui_NativeTilePreviewBridge_exportFinalizedProjection(
        env,
        receiver,
        sourceFd,
        outputFd,
        projectionKind,
        maxSourceResidentBytes,
        maxLogicalResidentBytes);
}
