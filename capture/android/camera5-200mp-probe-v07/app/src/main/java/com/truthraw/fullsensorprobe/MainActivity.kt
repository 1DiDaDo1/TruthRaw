package com.truthraw.fullsensorprobe

import android.Manifest
import android.app.Activity
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.ImageFormat
import android.hardware.camera2.*
import android.hardware.camera2.params.OutputConfiguration
import android.hardware.camera2.params.SessionConfiguration
import android.media.Image
import android.media.ImageReader
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.util.Size
import android.view.View
import android.widget.TextView
import androidx.core.content.FileProvider
import java.io.File
import java.io.FileOutputStream
import java.security.MessageDigest
import java.util.concurrent.Executor
import org.json.JSONArray
import org.json.JSONObject

/**
 * TruthRaw Camera 5 200MP Probe v0.7.
 * Independent Camera2 evidence capture. No TruthRaw canonical reconstruction code is modified here.
 */
class MainActivity : Activity() {
    private lateinit var cm: CameraManager
    private lateinit var logView: TextView
    private val thread = HandlerThread("truthraw-camera").apply { start() }
    private val handler = Handler(thread.looper)
    private val executor = Executor { r -> handler.post(r) }
    private var latestInventory: JSONObject? = null
    private var latestInventoryFile: File? = null
    private var latestManifestFile: File? = null
    private var latestRawFile: File? = null
    private var latestDngFile: File? = null
    private val targetTeleFocalMm = 22.48

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        cm = getSystemService(Context.CAMERA_SERVICE) as CameraManager
        logView = findViewById(R.id.log)
        findViewById<View>(R.id.run).setOnClickListener { ensurePermission { runFullProof() } }
        findViewById<View>(R.id.dump).setOnClickListener { ensurePermission { handler.post { saveInventory(buildInventory()) } } }
        findViewById<View>(R.id.capture).setOnClickListener { ensurePermission { attemptBestTeleCapture() } }
        findViewById<View>(R.id.share).setOnClickListener { shareEvidence() }
    }

    private var pending: (() -> Unit)? = null
    private fun ensurePermission(action: () -> Unit) {
        if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) action()
        else { pending = action; requestPermissions(arrayOf(Manifest.permission.CAMERA), 9) }
    }
    override fun onRequestPermissionsResult(rc: Int, p: Array<out String>, g: IntArray) {
        super.onRequestPermissionsResult(rc,p,g)
        if (rc==9 && g.firstOrNull()==PackageManager.PERMISSION_GRANTED) pending?.invoke()
        pending=null
    }

    private fun log(s: String) = runOnUiThread { logView.append(s + "\n") }
    private fun size(s: Size?): JSONArray? = s?.let { JSONArray(listOf(it.width,it.height)) }
    private fun rect(r: android.graphics.Rect?): JSONArray? = r?.let { JSONArray(listOf(it.left,it.top,it.right,it.bottom)) }
    private fun sizes(xs: Array<Size>?): JSONArray = JSONArray((xs ?: emptyArray()).sortedByDescending { it.width.toLong()*it.height }.map { listOf(it.width,it.height) })
    private fun caps(c: CameraCharacteristics): Set<Int> = c.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES)?.toSet() ?: emptySet()
    private fun hasRequestKey(c: CameraCharacteristics, name: String) = c.availableCaptureRequestKeys?.any { it.name == name } == true
    private fun hasResultKey(c: CameraCharacteristics, name: String) = c.availableCaptureResultKeys?.any { it.name == name } == true
    private fun outDir(): File = File(getExternalFilesDir(null),"truthraw_camera5_200mp_v07").apply { mkdirs() }

    private fun formats(c: CameraCharacteristics, maximum:Boolean): JSONObject {
        val m = if (maximum) c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
                else c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
        val o=JSONObject()
        if (m == null) return o
        listOf(
            "RAW_SENSOR" to ImageFormat.RAW_SENSOR,
            "RAW10" to ImageFormat.RAW10,
            "RAW12" to ImageFormat.RAW12
        ).forEach { (n,f) ->
            try { o.put(n, sizes(m.getOutputSizes(f))) } catch (_:Throwable) { o.put(n, JSONArray()) }
        }
        return o
    }

    private fun formatsHighResolution(c: CameraCharacteristics): JSONObject {
        val m = c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
        val o = JSONObject()
        if (m == null) return o
        listOf(
            "RAW_SENSOR" to ImageFormat.RAW_SENSOR,
            "RAW10" to ImageFormat.RAW10,
            "RAW12" to ImageFormat.RAW12
        ).forEach { (n,f) ->
            try { o.put(n, sizes(m.getHighResolutionOutputSizes(f))) } catch (_:Throwable) { o.put(n, JSONArray()) }
        }
        return o
    }

    private fun cameraRecord(id:String, parentLogical:String?=null):JSONObject {
        val c=cm.getCameraCharacteristics(id)
        val o=JSONObject()
        o.put("camera_id",id)
        o.put("parent_logical_id",parentLogical)
        o.put("physical_ids",JSONArray(c.physicalCameraIds.toList()))
        o.put("capabilities",JSONArray(caps(c).toList()))
        o.put("focal_lengths_mm",JSONArray((c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)?: floatArrayOf()).toList()))
        o.put("apertures",JSONArray((c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_APERTURES)?: floatArrayOf()).toList()))
        o.put("lens_facing",c.get(CameraCharacteristics.LENS_FACING))
        o.put("sensor_physical_size_mm", sizeF(c.get(CameraCharacteristics.SENSOR_INFO_PHYSICAL_SIZE)))
        o.put("cfa_arrangement",c.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT))
        o.put("white_level",c.get(CameraCharacteristics.SENSOR_INFO_WHITE_LEVEL))
        o.put("black_level_pattern",c.get(CameraCharacteristics.SENSOR_BLACK_LEVEL_PATTERN)?.let { bl -> JSONArray(listOf(bl.getOffsetForIndex(0,0),bl.getOffsetForIndex(1,0),bl.getOffsetForIndex(0,1),bl.getOffsetForIndex(1,1))) })
        o.put("pixel_array",size(c.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE)))
        o.put("active_array",rect(c.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE)))
        o.put("pre_correction_active_array",rect(c.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE)))
        o.put("pixel_array_maximum_resolution",size(c.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE_MAXIMUM_RESOLUTION)))
        o.put("active_array_maximum_resolution",rect(c.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION)))
        o.put("pre_correction_active_array_maximum_resolution",rect(c.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION)))
        o.put("sensor_info_binning_factor",size(c.get(CameraCharacteristics.SENSOR_INFO_BINNING_FACTOR)))
        o.put("lens_shading_applied_to_raw",c.get(CameraCharacteristics.SENSOR_INFO_LENS_SHADING_APPLIED))
        o.put("default_raw",formats(c,false))
        o.put("maximum_resolution_raw",formats(c,true))
        o.put("maximum_resolution_highres_raw",formatsHighResolution(c))
        o.put("sensor_pixel_mode_request_key_present", hasRequestKey(c,"android.sensor.pixelMode"))
        o.put("raw_binning_result_key_present", hasResultKey(c,"android.sensor.rawBinningFactorUsed"))
        return o
    }

    private fun sizeF(s: android.util.SizeF?): JSONArray? = s?.let { JSONArray(listOf(it.width.toDouble(),it.height.toDouble())) }

    private fun buildInventory():JSONObject {
        val root=JSONObject()
        root.put("schema","TruthRawCamera2Inventory/0.7")
        root.put("evidence_class","DEVICE_RUNTIME_CAPABILITY_DUMP")
        root.put("device",JSONObject().put("manufacturer",android.os.Build.MANUFACTURER).put("model",android.os.Build.MODEL).put("device",android.os.Build.DEVICE).put("sdk",android.os.Build.VERSION.SDK_INT))
        root.put("target_tele_focal_mm",targetTeleFocalMm)
        val cams=JSONArray(); val seen=mutableSetOf<String>()
        for (id in cm.cameraIdList) {
            try { cams.put(cameraRecord(id,null)) } catch(t:Throwable) { cams.put(JSONObject().put("camera_id",id).put("query_error",t.toString())) }
            seen.add(id)
            try {
                val c=cm.getCameraCharacteristics(id)
                for (pid in c.physicalCameraIds) if (seen.add("$id::$pid")) {
                    try { cams.put(cameraRecord(pid,id)) } catch(t:Throwable) { cams.put(JSONObject().put("camera_id",pid).put("parent_logical_id",id).put("query_error",t.toString())) }
                }
            } catch (_:Throwable) {}
        }
        root.put("cameras",cams)
        return root
    }

    private fun saveInventory(root:JSONObject) {
        latestInventory=root
        val f=File(outDir(),"truthraw_camera2_inventory_v07.json")
        f.writeText(root.toString(2)); latestInventoryFile=f
        log("Inventory written: ${f.absolutePath}")
        log(summarize(root))
    }

    private fun runFullProof() {
        handler.post {
            try {
                log("v0.7: inventory -> exact Camera 5 -> HIGH-RES 16320x12288 RAW_SENSOR -> capture")
                val inv=buildInventory(); saveInventory(inv)
                attemptBestTeleCapture()
            } catch(t:Throwable) { log("Full proof ERROR: $t") }
        }
    }

    private data class Candidate(
        val id:String,val parent:String?,val focal:Double,val lensFacing:Int?,
        val fmt:Int,val fmtName:String,val size:Size,val record:JSONObject
    )

    private fun formatRank(name:String)=when(name){"RAW_SENSOR"->3;"RAW12"->2;"RAW10"->1;else->0}
    private fun bestCandidate():Candidate? {
        val inv=latestInventory ?: return null
        val candidates=mutableListOf<Candidate>()
        val cameras=inv.getJSONArray("cameras")
        for(i in 0 until cameras.length()) {
            val o=cameras.getJSONObject(i); if(o.has("query_error")) continue
            // v0.7 is deliberately fail-closed for the device-proven tele route.
            if(o.optString("camera_id") != "5") continue
            val foc=o.optJSONArray("focal_lengths_mm")
            val focalValues=mutableListOf<Double>(); if(foc!=null) for(j in 0 until foc.length()) focalValues.add(foc.optDouble(j,Double.NaN))
            val focal=focalValues.filter{it.isFinite()}.minByOrNull { kotlin.math.abs(it-targetTeleFocalMm) } ?: 0.0
            if(kotlin.math.abs(focal-targetTeleFocalMm) > 0.05) continue
            val facing=if(o.isNull("lens_facing")) null else o.optInt("lens_facing")
            val sources=listOf(
                o.optJSONObject("maximum_resolution_highres_raw"),
                o.optJSONObject("maximum_resolution_raw")
            )
            for(max in sources) {
                if(max==null) continue
                for((name,fmt) in listOf("RAW_SENSOR" to ImageFormat.RAW_SENSOR)) {
                    val a=max.optJSONArray(name) ?: continue
                    for(j in 0 until a.length()) {
                        val wh=a.getJSONArray(j); val w=wh.getInt(0); val h=wh.getInt(1)
                        // Only the full Camera-5 maximum lattice closes the v0.7 target gate.
                        if(w != 16320 || h != 12288) continue
                        candidates.add(Candidate("5",null,focal,facing,fmt,name,Size(w,h),o))
                    }
                }
            }
        }
        if(candidates.isEmpty()) return null
        return candidates.sortedWith(compareByDescending<Candidate> { formatRank(it.fmtName) }).first()
    }

    private fun attemptBestTeleCapture() {
        if (latestInventory==null) { handler.post { saveInventory(buildInventory()); attemptBestTeleCapture() }; return }
        val c=bestCandidate()
        if(c==null) { log("No maximum-resolution RAW stream exposed. FULL_SENSOR gate stays blocked."); return }
        log("Selected tele candidate: id=${c.id} parent=${c.parent} focal=${c.focal} ${c.fmtName} ${c.size.width}x${c.size.height}")
        val selected=JSONObject().put("camera_id",c.id).put("parent_logical_id",c.parent).put("focal_mm",c.focal).put("format",c.fmtName).put("size",JSONArray(listOf(c.size.width,c.size.height)))
        File(outDir(),"truthraw_selected_candidate_v07.json").writeText(selected.toString(2))
        val openId=c.parent ?: c.id
        try {
            @Suppress("MissingPermission")
            cm.openCamera(openId, object:CameraDevice.StateCallback(){
                override fun onOpened(device:CameraDevice){ createCapture(device,c) }
                override fun onDisconnected(device:CameraDevice){log("Camera disconnected");device.close()}
                override fun onError(device:CameraDevice,error:Int){log("Open camera error=$error");device.close()}
            },handler)
        } catch(t:Throwable){log("openCamera exception: $t")}
    }

    private fun createCapture(device:CameraDevice,c:Candidate) {
        val reader=ImageReader.newInstance(c.size.width,c.size.height,c.fmt,1)
        var captureResult:TotalCaptureResult?=null
        var image:Image?=null
        var finished=false
        fun cleanup(){ try{image?.close()}catch(_:Throwable){};try{reader.close()}catch(_:Throwable){};try{device.close()}catch(_:Throwable){} }
        fun finishIfReady(){
            if(finished) return
            val im=image ?: return; val result=captureResult ?: return
            finished=true
            try { persistCapture(device,c,im,result) } catch(t:Throwable){ log("persist ERROR: $t") } finally { cleanup() }
        }
        reader.setOnImageAvailableListener({ r -> try { image=r.acquireNextImage(); finishIfReady() } catch(t:Throwable){log("image error $t")} },handler)
        val out=OutputConfiguration(reader.surface)
        if(c.parent!=null) out.setPhysicalCameraId(c.id)
        out.addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
        val sess=SessionConfiguration(SessionConfiguration.SESSION_REGULAR, listOf(out), executor, object:CameraCaptureSession.StateCallback(){
            override fun onConfigured(session:CameraCaptureSession){
                try {
                    val b=device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
                    b.addTarget(reader.surface)
                    b.set(CaptureRequest.SENSOR_PIXEL_MODE,CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
                    setIfSupported(b,CaptureRequest.CONTROL_ENABLE_ZSL,false,cm.getCameraCharacteristics(c.id))
                    applyEvidenceControls(b,cm.getCameraCharacteristics(c.id))
                    session.capture(b.build(),object:CameraCaptureSession.CaptureCallback(){
                        override fun onCaptureCompleted(s:CameraCaptureSession,req:CaptureRequest,res:TotalCaptureResult){captureResult=res;finishIfReady()}
                        override fun onCaptureFailed(s:CameraCaptureSession,req:CaptureRequest,f:CaptureFailure){log("Capture failed reason=${f.reason}");cleanup()}
                    },handler)
                }catch(t:Throwable){log("capture request error $t");cleanup()}
            }
            override fun onConfigureFailed(session:CameraCaptureSession){log("MAX_RES session configuration failed");cleanup()}
        })
        try {
            val support=try { device.isSessionConfigurationSupported(sess) } catch(t:Throwable) { null }
            log("SessionConfiguration supported=$support")
            File(outDir(),"truthraw_session_support_v07.json").writeText(JSONObject().put("schema","TruthRawSessionSupport/0.7").put("camera_id",c.id).put("parent",c.parent).put("format",c.fmtName).put("size",JSONArray(listOf(c.size.width,c.size.height))).put("is_session_configuration_supported",support).toString(2))
            if(support==false){ log("Fail closed: device reports max-res session unsupported"); cleanup(); return }
            device.createCaptureSession(sess)
        } catch(t:Throwable){log("createCaptureSession exception $t");cleanup()}
    }

    private fun <T> setIfSupported(b:CaptureRequest.Builder,key:CaptureRequest.Key<T>,value:T,c:CameraCharacteristics){
        try { if(c.availableCaptureRequestKeys?.contains(key)==true) b.set(key,value) } catch(_:Throwable){}
    }
    private fun modeAvailable(xs:IntArray?,value:Int)=xs?.contains(value)==true
    private fun applyEvidenceControls(b:CaptureRequest.Builder,c:CameraCharacteristics){
        if(modeAvailable(c.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES),CaptureRequest.NOISE_REDUCTION_MODE_OFF))
            setIfSupported(b,CaptureRequest.NOISE_REDUCTION_MODE,CaptureRequest.NOISE_REDUCTION_MODE_OFF,c)
        if(modeAvailable(c.get(CameraCharacteristics.HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES),CaptureRequest.HOT_PIXEL_MODE_OFF))
            setIfSupported(b,CaptureRequest.HOT_PIXEL_MODE,CaptureRequest.HOT_PIXEL_MODE_OFF,c)
        if(modeAvailable(c.get(CameraCharacteristics.SHADING_AVAILABLE_MODES),CaptureRequest.SHADING_MODE_OFF))
            setIfSupported(b,CaptureRequest.SHADING_MODE,CaptureRequest.SHADING_MODE_OFF,c)
        if(modeAvailable(c.get(CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES),CaptureRequest.EDGE_MODE_OFF))
            setIfSupported(b,CaptureRequest.EDGE_MODE,CaptureRequest.EDGE_MODE_OFF,c)
        if(modeAvailable(c.get(CameraCharacteristics.COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES),CaptureRequest.COLOR_CORRECTION_ABERRATION_MODE_OFF))
            setIfSupported(b,CaptureRequest.COLOR_CORRECTION_ABERRATION_MODE,CaptureRequest.COLOR_CORRECTION_ABERRATION_MODE_OFF,c)
        if(modeAvailable(c.get(CameraCharacteristics.DISTORTION_CORRECTION_AVAILABLE_MODES),CaptureRequest.DISTORTION_CORRECTION_MODE_OFF))
            setIfSupported(b,CaptureRequest.DISTORTION_CORRECTION_MODE,CaptureRequest.DISTORTION_CORRECTION_MODE_OFF,c)
        if(modeAvailable(c.get(CameraCharacteristics.CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES),CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE_OFF))
            setIfSupported(b,CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE,CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE_OFF,c)
        if(modeAvailable(c.get(CameraCharacteristics.STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES),CaptureRequest.STATISTICS_LENS_SHADING_MAP_MODE_ON))
            setIfSupported(b,CaptureRequest.STATISTICS_LENS_SHADING_MAP_MODE,CaptureRequest.STATISTICS_LENS_SHADING_MAP_MODE_ON,c)
    }

    private fun captureControlResult(r:CaptureResult):JSONObject = JSONObject()
        .put("noise_reduction_mode",r.get(CaptureResult.NOISE_REDUCTION_MODE))
        .put("hot_pixel_mode",r.get(CaptureResult.HOT_PIXEL_MODE))
        .put("shading_mode",r.get(CaptureResult.SHADING_MODE))
        .put("edge_mode",r.get(CaptureResult.EDGE_MODE))
        .put("aberration_mode",r.get(CaptureResult.COLOR_CORRECTION_ABERRATION_MODE))
        .put("distortion_correction_mode",r.get(CaptureResult.DISTORTION_CORRECTION_MODE))
        .put("ois_mode",r.get(CaptureResult.LENS_OPTICAL_STABILIZATION_MODE))
        .put("video_stabilization_mode",r.get(CaptureResult.CONTROL_VIDEO_STABILIZATION_MODE))

    private fun persistCapture(device:CameraDevice,c:Candidate,image:Image,result:TotalCaptureResult){
        val plane=image.planes[0]
        val pixelStride=try{plane.pixelStride}catch(_:Throwable){-1}
        val rowStride=plane.rowStride
        val sourceBuffer=plane.buffer.duplicate().apply { rewind() }
        val accessibleBytes=sourceBuffer.remaining().toLong()
        val expectedContiguous=image.width.toLong()*image.height.toLong()*2L
        val contiguous=(c.fmt==ImageFormat.RAW_SENSOR && pixelStride==2 && rowStride==image.width*2 && accessibleBytes==expectedContiguous)
        val base="truthraw_camera5_${c.size.width}x${c.size.height}_${c.fmtName}_v07"
        val raw=File(outDir(), if(contiguous) "$base.rawsensor" else "$base.rawbuffer")

        // Critical v0.7 rule: never allocate a ~401 MB Java/Kotlin ByteArray.
        // Hash and persist the direct Image.Plane buffer using duplicate ByteBuffers.
        val md=MessageDigest.getInstance("SHA-256")
        md.update(plane.buffer.duplicate().apply { rewind() })
        val hash=md.digest().joinToString(""){"%02x".format(it)}
        FileOutputStream(raw).channel.use { ch ->
            val w=plane.buffer.duplicate().apply { rewind() }
            while(w.hasRemaining()) ch.write(w)
            ch.force(true)
        }
        latestRawFile=raw
        val cc=cm.getCameraCharacteristics(c.id)
        val physicalResult=if(c.parent!=null) result.physicalCameraTotalResults[c.id] else result
        val effectiveResult=physicalResult ?: result
        val sensorTs=effectiveResult.get(CaptureResult.SENSOR_TIMESTAMP)

        var dngFile:File?=null; var dngError:String?=null
        if(c.fmt==ImageFormat.RAW_SENSOR){
            try {
                val dng=File(outDir(),"$base.dng")
                FileOutputStream(dng).use { os -> DngCreator(cc,effectiveResult).use { creator -> creator.setDescription("TruthRaw v0.7 Camera 5 full 200MP maximum-resolution Camera2 evidence DNG; raw payload SHA-256=$hash"); creator.writeImage(os,image) } }
                dngFile=dng; latestDngFile=dng
                log("DNG written: ${dng.absolutePath}")
            } catch(t:Throwable){ dngError=t.toString(); log("DngCreator did not publish DNG: $dngError") }
        }

        val m=JSONObject()
        m.put("schema","TruthRawCamera2RuntimeCapture/0.7")
        m.put("evidence_class","DEVICE_RUNTIME_CAPTURE")
        m.put("scientific_boundary","Application-visible maximum-resolution RAW evidence. Does not prove absence of upstream sensor/HAL processing or one ADC code per physical photodiode.")
        m.put("device",JSONObject().put("make",android.os.Build.MANUFACTURER).put("model",android.os.Build.MODEL).put("device",android.os.Build.DEVICE).put("sdk",android.os.Build.VERSION.SDK_INT))
        m.put("opened_camera_id",device.id);m.put("physical_camera_id",c.id);m.put("parent_logical_id",c.parent);m.put("focal_mm",c.focal)
        m.put("source_identity_sha256",hash)
        m.put("camera_characteristics",c.record)
        m.put("default_raw_sizes", formats(cc,false).optJSONArray(c.fmtName) ?: JSONArray())
        m.put("maximum_resolution_raw_sizes", formats(cc,true).optJSONArray(c.fmtName) ?: JSONArray())
        m.put("maximum_resolution_highres_raw_sizes", formatsHighResolution(cc).optJSONArray(c.fmtName) ?: JSONArray())
        m.put("sensor_info_pixel_array_size",size(cc.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE)))
        m.put("sensor_info_pixel_array_size_maximum_resolution",size(cc.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE_MAXIMUM_RESOLUTION)))
        m.put("sensor_info_active_array_size",rect(cc.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE)))
        m.put("sensor_info_active_array_size_maximum_resolution",rect(cc.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION)))
        m.put("sensor_info_pre_correction_active_array_size",rect(cc.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE)))
        m.put("sensor_info_pre_correction_active_array_size_maximum_resolution",rect(cc.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION)))
        m.put("sensor_info_binning_factor",size(cc.get(CameraCharacteristics.SENSOR_INFO_BINNING_FACTOR)))
        m.put("requested",JSONObject().put("sensor_pixel_mode","MAXIMUM_RESOLUTION").put("zsl",false).put("raw_format",c.fmtName).put("raw_size",JSONArray(listOf(c.size.width,c.size.height))))
        val cr=JSONObject()
        cr.put("result_camera_id",effectiveResult.cameraId)
        cr.put("physical_result_present",physicalResult!=null)
        cr.put("sensor_pixel_mode",when(effectiveResult.get(CaptureResult.SENSOR_PIXEL_MODE)){CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION->"MAXIMUM_RESOLUTION";CameraMetadata.SENSOR_PIXEL_MODE_DEFAULT->"DEFAULT";else->null})
        cr.put("sensor_raw_binning_factor_used",effectiveResult.get(CaptureResult.SENSOR_RAW_BINNING_FACTOR_USED))
        cr.put("active_physical_camera_id",result.get(CaptureResult.LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID))
        cr.put("sensor_timestamp_ns",sensorTs);cr.put("frame_number",effectiveResult.frameNumber)
        cr.put("exposure_time_ns",effectiveResult.get(CaptureResult.SENSOR_EXPOSURE_TIME));cr.put("sensitivity_iso",effectiveResult.get(CaptureResult.SENSOR_SENSITIVITY))
        cr.put("noise_profile",effectiveResult.get(CaptureResult.SENSOR_NOISE_PROFILE)?.let { np -> JSONArray(np.flatMap { listOf(it.first,it.second) }) })
        cr.put("dynamic_black_level",effectiveResult.get(CaptureResult.SENSOR_DYNAMIC_BLACK_LEVEL)?.let { JSONArray(it.toList()) })
        cr.put("dynamic_white_level",effectiveResult.get(CaptureResult.SENSOR_DYNAMIC_WHITE_LEVEL))
        cr.put("applied_controls",captureControlResult(effectiveResult))
        m.put("capture_result",cr)
        m.put("raw_output",JSONObject()
            .put("file",raw.name).put("format",c.fmtName).put("width",image.width).put("height",image.height)
            .put("row_stride",rowStride).put("pixel_stride",pixelStride).put("payload_bytes",accessibleBytes)
            .put("expected_contiguous_bytes",expectedContiguous).put("canonical_contiguous_rawsensor",contiguous)
            .put("payload_sha256",hash).put("image_timestamp_ns",image.timestamp)
            .put("timestamp_matches_capture_result",sensorTs!=null && image.timestamp==sensorTs))
        m.put("dng_output",JSONObject().put("attempted",c.fmt==ImageFormat.RAW_SENSOR).put("file",dngFile?.name).put("sha256",dngFile?.let{sha256File(it)}).put("bytes",dngFile?.length()).put("error",dngError).put("semantics","Android DngCreator convenience container; exact app-visible RAW buffer remains the primary evidence payload."))
        val mf=File(outDir(),"truthraw_camera5_200mp_capture_manifest_v07.json");mf.writeText(m.toString(2));latestManifestFile=mf
        log("MAX RAW written ${raw.absolutePath}")
        log("Manifest ${mf.absolutePath}")
        log("SHA256=$hash timestampMatch=${sensorTs!=null && image.timestamp==sensorTs} physicalResult=${physicalResult!=null}")
        log("Use Share evidence or return inventory + manifest + .bin/.dng to TruthRaw lab.")
    }

    private fun shareEvidence(){
        val files=listOfNotNull(latestInventoryFile,latestManifestFile,latestDngFile,latestRawFile).filter{it.exists()}
        if(files.isEmpty()){log("Nothing to share yet; run the proof first.");return}
        try{
            val uris=ArrayList<Uri>(files.map{FileProvider.getUriForFile(this,"${packageName}.files",it)})
            val i=Intent(Intent.ACTION_SEND_MULTIPLE).apply{
                type="application/octet-stream";putParcelableArrayListExtra(Intent.EXTRA_STREAM,uris);addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                putExtra(Intent.EXTRA_SUBJECT,"TruthRaw Camera 5 200MP v0.7 evidence")
            }
            startActivity(Intent.createChooser(i,"Share TruthRaw evidence"))
        }catch(t:Throwable){log("Share ERROR: $t")}
    }

    private fun summarize(root:JSONObject):String{
        val a=root.getJSONArray("cameras");val b=StringBuilder()
        for(i in 0 until a.length()){
            val o=a.getJSONObject(i);if(o.has("query_error"))continue
            val f=o.optJSONArray("focal_lengths_mm");val mr=o.optJSONObject("maximum_resolution_raw")
            val has=mr?.keys()?.asSequence()?.any{(mr.optJSONArray(it)?.length()?:0)>0}==true
            if(has||(f?.length()?:0)>0)b.append("id=${o.optString("camera_id")} parent=${o.optString("parent_logical_id")} focal=$f MAX_RAW=$mr\n")
        }
        return b.toString()
    }
    private fun sanitize(s:String)=s.replace(Regex("[^A-Za-z0-9_.-]"),"_")
    private fun sha256File(f:File):String{val md=MessageDigest.getInstance("SHA-256");f.inputStream().use{ins->val b=ByteArray(1 shl 20);while(true){val n=ins.read(b);if(n<0)break;md.update(b,0,n)}};return md.digest().joinToString(""){"%02x".format(it)}}
}
