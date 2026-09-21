#include "truthnegative_vulkan_dense_v0_1.h"

#include "truthnegative_dense_4x_spv.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <sstream>
#include <string>
#include <vector>

namespace truthraw::truthnegative_vulkan_dense::v0_1 {
namespace {

namespace generated = truthraw::truthnegative_vulkan_dense::generated;

constexpr std::uint32_t kLocalSizeX = 16u;
constexpr std::uint32_t kLocalSizeY = 16u;

struct Push final {
    std::uint32_t sourceFullWidth;
    std::uint32_t sourceFullHeight;
    std::uint32_t patchOriginX;
    std::uint32_t patchOriginY;
    std::uint32_t patchWidth;
    std::uint32_t patchHeight;
    std::uint32_t targetOriginX;
    std::uint32_t targetOriginY;
    std::uint32_t targetWidth;
    std::uint32_t targetHeight;
};
static_assert(sizeof(Push) == 40u);

struct Buffer final {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0u;
};

std::uint32_t clamp_index(int v, std::uint32_t limit) noexcept {
    if (limit == 0u || v <= 0) return 0u;
    const auto u = static_cast<std::uint32_t>(v);
    return u >= limit ? limit - 1u : u;
}

float axis_fraction(std::uint32_t targetCoord, int& base) noexcept {
    const std::uint32_t n = targetCoord >> 2u;
    switch (targetCoord & 3u) {
        case 0u: base = static_cast<int>(n) - 1; return 0.625f;
        case 1u: base = static_cast<int>(n) - 1; return 0.875f;
        case 2u: base = static_cast<int>(n); return 0.125f;
        default: base = static_cast<int>(n); return 0.375f;
    }
}

bool cpu_reference(
    const PatchRequest& request,
    const float* sourceRgb,
    std::size_t sourceFloatCount,
    float* targetRgb,
    std::size_t targetFloatCount) noexcept {
    if (!sourceRgb || !targetRgb ||
        request.sourceFullWidth == 0u || request.sourceFullHeight == 0u ||
        request.patchWidth == 0u || request.patchHeight == 0u ||
        request.targetWidth == 0u || request.targetHeight == 0u) return false;

    const std::size_t expectedSource =
        static_cast<std::size_t>(request.patchWidth) * request.patchHeight * 3u;
    const std::size_t expectedTarget =
        static_cast<std::size_t>(request.targetWidth) * request.targetHeight * 3u;
    if (sourceFloatCount != expectedSource || targetFloatCount != expectedTarget) {
        return false;
    }

    for (std::uint32_t oy=0u; oy<request.targetHeight; ++oy) {
        const auto ty = request.targetOriginY + oy;
        int by=0;
        const float fy = axis_fraction(ty, by);
        const auto y0 = clamp_index(by, request.sourceFullHeight);
        const auto y1 = clamp_index(by + 1, request.sourceFullHeight);
        if (y0 < request.patchOriginY || y1 < request.patchOriginY) return false;
        const auto ly0 = y0 - request.patchOriginY;
        const auto ly1 = y1 - request.patchOriginY;
        if (ly0 >= request.patchHeight || ly1 >= request.patchHeight) return false;

        for (std::uint32_t ox=0u; ox<request.targetWidth; ++ox) {
            const auto tx = request.targetOriginX + ox;
            int bx=0;
            const float fx = axis_fraction(tx, bx);
            const auto x0 = clamp_index(bx, request.sourceFullWidth);
            const auto x1 = clamp_index(bx + 1, request.sourceFullWidth);
            if (x0 < request.patchOriginX || x1 < request.patchOriginX) return false;
            const auto lx0 = x0 - request.patchOriginX;
            const auto lx1 = x1 - request.patchOriginX;
            if (lx0 >= request.patchWidth || lx1 >= request.patchWidth) return false;

            for (std::uint32_t c=0u; c<3u; ++c) {
                const auto i00 =
                    (static_cast<std::size_t>(ly0)*request.patchWidth+lx0)*3u+c;
                const auto i10 =
                    (static_cast<std::size_t>(ly0)*request.patchWidth+lx1)*3u+c;
                const auto i01 =
                    (static_cast<std::size_t>(ly1)*request.patchWidth+lx0)*3u+c;
                const auto i11 =
                    (static_cast<std::size_t>(ly1)*request.patchWidth+lx1)*3u+c;
                const float p00=sourceRgb[i00];
                const float p10=sourceRgb[i10];
                const float p01=sourceRgb[i01];
                const float p11=sourceRgb[i11];
                const float top = p00 + (p10 - p00) * fx;
                const float bottom = p01 + (p11 - p01) * fx;
                const float value = top + (bottom - top) * fy;
                const auto out =
                    (static_cast<std::size_t>(oy)*request.targetWidth+ox)*3u+c;
                targetRgb[out]=value;
            }
        }
    }
    return true;
}

int device_score(VkPhysicalDeviceType type) noexcept {
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 4;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return 3;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return 2;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER: return 1;
        case VK_PHYSICAL_DEVICE_TYPE_CPU: return -100;
        default: return 0;
    }
}

}  // namespace

struct Backend::Impl final {
    Probe probe{};
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    std::uint32_t queueFamily = 0u;
    VkPhysicalDeviceMemoryProperties memoryProperties{};

    VkShaderModule shader = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    Buffer sourceBuffer{};
    Buffer targetBuffer{};
    std::mutex mutex;

    ~Impl() {
        if (device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device);
            if (sourceBuffer.buffer != VK_NULL_HANDLE) vkDestroyBuffer(device, sourceBuffer.buffer, nullptr);
            if (sourceBuffer.memory != VK_NULL_HANDLE) vkFreeMemory(device, sourceBuffer.memory, nullptr);
            if (targetBuffer.buffer != VK_NULL_HANDLE) vkDestroyBuffer(device, targetBuffer.buffer, nullptr);
            if (targetBuffer.memory != VK_NULL_HANDLE) vkFreeMemory(device, targetBuffer.memory, nullptr);
            if (commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(device, commandPool, nullptr);
            if (descriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            if (pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, pipeline, nullptr);
            if (pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            if (descriptorLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
            if (shader != VK_NULL_HANDLE) vkDestroyShaderModule(device, shader, nullptr);
            vkDestroyDevice(device, nullptr);
        }
        if (instance != VK_NULL_HANDLE) vkDestroyInstance(instance, nullptr);
    }

    std::uint32_t memoryType(
        std::uint32_t bits,
        VkMemoryPropertyFlags wanted) const noexcept {
        for (std::uint32_t i=0u; i<memoryProperties.memoryTypeCount; ++i) {
            if ((bits & (1u<<i)) != 0u &&
                (memoryProperties.memoryTypes[i].propertyFlags & wanted) == wanted) {
                return i;
            }
        }
        return std::numeric_limits<std::uint32_t>::max();
    }

    void destroyBuffer(Buffer& b) noexcept {
        if (device == VK_NULL_HANDLE) return;
        if (b.buffer != VK_NULL_HANDLE) vkDestroyBuffer(device,b.buffer,nullptr);
        if (b.memory != VK_NULL_HANDLE) vkFreeMemory(device,b.memory,nullptr);
        b={};
    }

    bool ensureBuffer(Buffer& out, VkDeviceSize size, std::string& error) {
        if (out.buffer != VK_NULL_HANDLE && out.size >= size) return true;
        destroyBuffer(out);

        VkBufferCreateInfo bi{};
        bi.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size=size;
        bi.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bi.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device,&bi,nullptr,&out.buffer)!=VK_SUCCESS) {
            error="vkCreateBuffer failed";
            return false;
        }

        VkMemoryRequirements req{};
        vkGetBufferMemoryRequirements(device,out.buffer,&req);
        const auto type=memoryType(
            req.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if(type==std::numeric_limits<std::uint32_t>::max()) {
            error="no HOST_VISIBLE|HOST_COHERENT Vulkan storage memory";
            destroyBuffer(out);
            return false;
        }

        VkMemoryAllocateInfo ai{};
        ai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize=req.size;
        ai.memoryTypeIndex=type;
        if(vkAllocateMemory(device,&ai,nullptr,&out.memory)!=VK_SUCCESS ||
           vkBindBufferMemory(device,out.buffer,out.memory,0u)!=VK_SUCCESS) {
            error="Vulkan storage allocation/bind failed";
            destroyBuffer(out);
            return false;
        }
        out.size=req.size;
        return true;
    }

    bool dispatch(
        const PatchRequest& request,
        const float* sourceRgb,
        std::size_t sourceFloatCount,
        float* targetRgb,
        std::size_t targetFloatCount,
        std::string& error) {
        const VkDeviceSize srcBytes =
            static_cast<VkDeviceSize>(sourceFloatCount*sizeof(float));
        const VkDeviceSize dstBytes =
            static_cast<VkDeviceSize>(targetFloatCount*sizeof(float));
        if(!ensureBuffer(sourceBuffer,srcBytes,error) ||
           !ensureBuffer(targetBuffer,dstBytes,error)) return false;

        void* mapped=nullptr;
        if(vkMapMemory(device,sourceBuffer.memory,0u,srcBytes,0u,&mapped)!=VK_SUCCESS) {
            error="vkMapMemory(source) failed";
            return false;
        }
        std::memcpy(mapped,sourceRgb,static_cast<std::size_t>(srcBytes));
        vkUnmapMemory(device,sourceBuffer.memory);

        VkDescriptorBufferInfo srcInfo{sourceBuffer.buffer,0u,srcBytes};
        VkDescriptorBufferInfo dstInfo{targetBuffer.buffer,0u,dstBytes};
        std::array<VkWriteDescriptorSet,2> writes{};
        for(auto& w:writes) w.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet=descriptorSet; writes[0].dstBinding=0u;
        writes[0].descriptorCount=1u; writes[0].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].pBufferInfo=&srcInfo;
        writes[1].dstSet=descriptorSet; writes[1].dstBinding=1u;
        writes[1].descriptorCount=1u; writes[1].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].pBufferInfo=&dstInfo;
        vkUpdateDescriptorSets(device,static_cast<std::uint32_t>(writes.size()),writes.data(),0u,nullptr);

        vkResetCommandBuffer(commandBuffer,0u);
        VkCommandBufferBeginInfo begin{};
        begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if(vkBeginCommandBuffer(commandBuffer,&begin)!=VK_SUCCESS) {
            error="vkBeginCommandBuffer failed"; return false;
        }
        vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
        vkCmdBindDescriptorSets(
            commandBuffer,VK_PIPELINE_BIND_POINT_COMPUTE,pipelineLayout,
            0u,1u,&descriptorSet,0u,nullptr);
        const Push push{
            request.sourceFullWidth,request.sourceFullHeight,
            request.patchOriginX,request.patchOriginY,
            request.patchWidth,request.patchHeight,
            request.targetOriginX,request.targetOriginY,
            request.targetWidth,request.targetHeight};
        vkCmdPushConstants(
            commandBuffer,pipelineLayout,VK_SHADER_STAGE_COMPUTE_BIT,
            0u,sizeof(push),&push);
        const auto gx=(request.targetWidth+kLocalSizeX-1u)/kLocalSizeX;
        const auto gy=(request.targetHeight+kLocalSizeY-1u)/kLocalSizeY;
        vkCmdDispatch(commandBuffer,gx,gy,1u);
        if(vkEndCommandBuffer(commandBuffer)!=VK_SUCCESS) {
            error="vkEndCommandBuffer failed"; return false;
        }

        VkSubmitInfo submit{};
        submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount=1u;
        submit.pCommandBuffers=&commandBuffer;
        if(vkQueueSubmit(queue,1u,&submit,VK_NULL_HANDLE)!=VK_SUCCESS ||
           vkQueueWaitIdle(queue)!=VK_SUCCESS) {
            error="Vulkan queue submit/wait failed"; return false;
        }

        mapped=nullptr;
        if(vkMapMemory(device,targetBuffer.memory,0u,dstBytes,0u,&mapped)!=VK_SUCCESS) {
            error="vkMapMemory(target) failed"; return false;
        }
        std::memcpy(targetRgb,mapped,static_cast<std::size_t>(dstBytes));
        vkUnmapMemory(device,targetBuffer.memory);
        return true;
    }

    bool init() {
        try {
            VkApplicationInfo app{};
            app.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;
            app.pApplicationName="TruthRawTruthNegativeDense";
            app.applicationVersion=1u;
            app.pEngineName="TruthRaw";
            app.engineVersion=1u;
            app.apiVersion=VK_API_VERSION_1_1;

            VkInstanceCreateInfo ici{};
            ici.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            ici.pApplicationInfo=&app;
            if(vkCreateInstance(&ici,nullptr,&instance)!=VK_SUCCESS) {
                probe.reason="vkCreateInstance failed";
                return false;
            }
            probe.loaderAvailable=true;

            std::uint32_t count=0u;
            if(vkEnumeratePhysicalDevices(instance,&count,nullptr)!=VK_SUCCESS || count==0u) {
                probe.reason="no Vulkan physical device";
                return false;
            }
            std::vector<VkPhysicalDevice> devices(count);
            if(vkEnumeratePhysicalDevices(instance,&count,devices.data())!=VK_SUCCESS) {
                probe.reason="Vulkan physical-device enumeration failed";
                return false;
            }

            int best=-1000;
            VkPhysicalDeviceProperties bestProps{};
            for(auto dev:devices) {
                VkPhysicalDeviceProperties props{};
                vkGetPhysicalDeviceProperties(dev,&props);
                std::uint32_t qCount=0u;
                vkGetPhysicalDeviceQueueFamilyProperties(dev,&qCount,nullptr);
                std::vector<VkQueueFamilyProperties> queues(qCount);
                if(qCount) vkGetPhysicalDeviceQueueFamilyProperties(dev,&qCount,queues.data());
                for(std::uint32_t q=0u;q<qCount;++q) {
                    if((queues[q].queueFlags&VK_QUEUE_COMPUTE_BIT)==0u || queues[q].queueCount==0u) continue;
                    const int score=device_score(props.deviceType);
                    if(score>best) {
                        best=score; physicalDevice=dev; queueFamily=q; bestProps=props;
                    }
                }
            }
            if(physicalDevice==VK_NULL_HANDLE ||
               bestProps.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU) {
                probe.reason="no hardware Vulkan compute device";
                return false;
            }
            probe.hardwareDeviceAvailable=true;
            probe.computeQueueAvailable=true;
            probe.vendorId=bestProps.vendorID;
            probe.deviceId=bestProps.deviceID;
            probe.driverVersion=bestProps.driverVersion;
            probe.deviceName=bestProps.deviceName;
            vkGetPhysicalDeviceMemoryProperties(physicalDevice,&memoryProperties);

            const float priority=1.0f;
            VkDeviceQueueCreateInfo qci{};
            qci.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qci.queueFamilyIndex=queueFamily;
            qci.queueCount=1u;
            qci.pQueuePriorities=&priority;
            VkDeviceCreateInfo dci{};
            dci.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            dci.queueCreateInfoCount=1u;
            dci.pQueueCreateInfos=&qci;
            if(vkCreateDevice(physicalDevice,&dci,nullptr,&device)!=VK_SUCCESS) {
                probe.reason="vkCreateDevice failed";
                return false;
            }
            vkGetDeviceQueue(device,queueFamily,0u,&queue);

            VkShaderModuleCreateInfo sm{};
            sm.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            sm.codeSize=generated::kDense4xSpirvBytes;
            sm.pCode=reinterpret_cast<const std::uint32_t*>(generated::kDense4xSpirv);
            if(vkCreateShaderModule(device,&sm,nullptr,&shader)!=VK_SUCCESS) {
                probe.reason="vkCreateShaderModule failed";
                return false;
            }

            std::array<VkDescriptorSetLayoutBinding,2> bindings{};
            for(std::uint32_t i=0u;i<2u;++i) {
                bindings[i].binding=i;
                bindings[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                bindings[i].descriptorCount=1u;
                bindings[i].stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;
            }
            VkDescriptorSetLayoutCreateInfo dl{};
            dl.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            dl.bindingCount=static_cast<std::uint32_t>(bindings.size());
            dl.pBindings=bindings.data();
            if(vkCreateDescriptorSetLayout(device,&dl,nullptr,&descriptorLayout)!=VK_SUCCESS) {
                probe.reason="vkCreateDescriptorSetLayout failed";
                return false;
            }

            VkPushConstantRange pc{};
            pc.stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;
            pc.offset=0u; pc.size=sizeof(Push);
            VkPipelineLayoutCreateInfo pl{};
            pl.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pl.setLayoutCount=1u; pl.pSetLayouts=&descriptorLayout;
            pl.pushConstantRangeCount=1u; pl.pPushConstantRanges=&pc;
            if(vkCreatePipelineLayout(device,&pl,nullptr,&pipelineLayout)!=VK_SUCCESS) {
                probe.reason="vkCreatePipelineLayout failed";
                return false;
            }

            VkPipelineShaderStageCreateInfo stage{};
            stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;
            stage.module=shader; stage.pName="main";
            VkComputePipelineCreateInfo cp{};
            cp.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            cp.stage=stage; cp.layout=pipelineLayout;
            if(vkCreateComputePipelines(device,VK_NULL_HANDLE,1u,&cp,nullptr,&pipeline)!=VK_SUCCESS) {
                probe.reason="vkCreateComputePipelines failed";
                return false;
            }
            probe.pipelineCreated=true;

            VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2u};
            VkDescriptorPoolCreateInfo dpi{};
            dpi.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            dpi.maxSets=1u; dpi.poolSizeCount=1u; dpi.pPoolSizes=&poolSize;
            if(vkCreateDescriptorPool(device,&dpi,nullptr,&descriptorPool)!=VK_SUCCESS) {
                probe.reason="vkCreateDescriptorPool failed"; return false;
            }
            VkDescriptorSetAllocateInfo dai{};
            dai.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            dai.descriptorPool=descriptorPool;
            dai.descriptorSetCount=1u; dai.pSetLayouts=&descriptorLayout;
            if(vkAllocateDescriptorSets(device,&dai,&descriptorSet)!=VK_SUCCESS) {
                probe.reason="vkAllocateDescriptorSets failed"; return false;
            }

            VkCommandPoolCreateInfo cpi{};
            cpi.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cpi.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            cpi.queueFamilyIndex=queueFamily;
            if(vkCreateCommandPool(device,&cpi,nullptr,&commandPool)!=VK_SUCCESS) {
                probe.reason="vkCreateCommandPool failed"; return false;
            }
            VkCommandBufferAllocateInfo cai{};
            cai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cai.commandPool=commandPool;
            cai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cai.commandBufferCount=1u;
            if(vkAllocateCommandBuffers(device,&cai,&commandBuffer)!=VK_SUCCESS) {
                probe.reason="vkAllocateCommandBuffers failed"; return false;
            }

            // Kernel-specific exactness gate.
            constexpr std::uint32_t sw=8u, sh=8u, tw=32u, th=32u;
            std::vector<float> source(static_cast<std::size_t>(sw)*sh*3u);
            for(std::uint32_t y=0u;y<sh;++y) {
                for(std::uint32_t x=0u;x<sw;++x) {
                    const auto i=(static_cast<std::size_t>(y)*sw+x)*3u;
                    source[i+0]=-0.125f+0.03125f*static_cast<float>(x)+
                                0.015625f*static_cast<float>(y);
                    source[i+1]=0.25f+0.046875f*static_cast<float>(x)+
                                0.0078125f*static_cast<float>(y);
                    source[i+2]=0.75f+0.0625f*static_cast<float>(x)+
                                0.0234375f*static_cast<float>(y);
                }
            }
            PatchRequest req{};
            req.sourceFullWidth=sw; req.sourceFullHeight=sh;
            req.patchWidth=sw; req.patchHeight=sh;
            req.targetWidth=tw; req.targetHeight=th;
            std::vector<float> cpu(static_cast<std::size_t>(tw)*th*3u);
            std::vector<float> gpu(cpu.size());
            if(!cpu_reference(req,source.data(),source.size(),cpu.data(),cpu.size())) {
                probe.reason="internal CPU self-test reference failed"; return false;
            }
            std::string dispatchError;
            if(!dispatch(req,source.data(),source.size(),gpu.data(),gpu.size(),dispatchError)) {
                probe.reason="Vulkan self-test dispatch failed: "+dispatchError; return false;
            }
            for(std::size_t i=0u;i<cpu.size();++i) {
                if(std::memcmp(&cpu[i],&gpu[i],sizeof(float))!=0) {
                    std::ostringstream msg;
                    msg<<"Vulkan dense self-test bit mismatch at component "<<i;
                    probe.reason=msg.str();
                    return false;
                }
            }
            probe.selfTestPassed=true;
            probe.exactScientificEligible=true;
            probe.reason="bit-exact CPU/Vulkan dense self-test passed";
            return true;
        } catch(const std::bad_alloc&) {
            probe.reason="Vulkan dense backend allocation failed";
            return false;
        } catch(...) {
            probe.reason="unexpected Vulkan dense backend initialization failure";
            return false;
        }
    }
};

Backend::Backend() noexcept : impl_(new (std::nothrow) Impl{}) {
    if(impl_) impl_->init();
}

Backend::~Backend() = default;

const Probe& Backend::probe() const noexcept {
    static const Probe unavailable{.reason="backend allocation failed"};
    return impl_ ? impl_->probe : unavailable;
}

bool Backend::available() const noexcept {
    return impl_ && impl_->probe.exactScientificEligible;
}

bool Backend::exactScientificEligible() const noexcept {
    return available();
}

const char* Backend::backendName() const noexcept {
    return "VULKAN_GENERIC_EXACT";
}

bool Backend::projectPatch(
    const PatchRequest& request,
    const float* sourceRgb,
    std::size_t sourceFloatCount,
    float* targetRgb,
    std::size_t targetFloatCount,
    std::string& error) noexcept {
    if(!available()) {
        error=probe().reason;
        return false;
    }
    if(!sourceRgb || !targetRgb ||
       request.sourceFullWidth==0u || request.sourceFullHeight==0u ||
       request.patchWidth==0u || request.patchHeight==0u ||
       request.targetWidth==0u || request.targetHeight==0u) {
        error="invalid Vulkan dense patch request";
        return false;
    }
    const auto expectedSource =
        static_cast<std::size_t>(request.patchWidth)*request.patchHeight*3u;
    const auto expectedTarget =
        static_cast<std::size_t>(request.targetWidth)*request.targetHeight*3u;
    if(sourceFloatCount!=expectedSource || targetFloatCount!=expectedTarget) {
        error="Vulkan dense patch float-count mismatch";
        return false;
    }
    try {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        std::string local;
        const bool ok=impl_->dispatch(
            request,sourceRgb,sourceFloatCount,targetRgb,targetFloatCount,local);
        if(!ok) error=local;
        return ok;
    } catch(...) {
        error="unexpected Vulkan dense dispatch failure";
        return false;
    }
}

}  // namespace truthraw::truthnegative_vulkan_dense::v0_1
