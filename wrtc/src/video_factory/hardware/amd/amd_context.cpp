//
// Created by Laky64 on 10/04/25.
//

#include <public/common/AMFFactory.h>
#include <rtc_base/logging.h>
#include <wrtc/video_factory/hardware/amd/amd_context.hpp>

namespace amd {
    std::mutex AMDContext::_mutex{};
    bool AMDContext::initialized = false;
    rtc::scoped_refptr<AMDContext> AMDContext::_default = nullptr;

    rtc::scoped_refptr<AMDContext> AMDContext::GetOrCreateDefault() {
        if (const AMF_RESULT res = g_AMFFactory.Init(); res != AMF_OK) {
            RTC_LOG(LS_ERROR) << "Failed to initialize AMF factory: " << res;
            return nullptr;
        }
        std::lock_guard lock(_mutex);
        if (initialized) {
            return _default;
        }
        initialized = true;
        RTC_LOG(LS_INFO) << "AMF factory initialized";
        return _default = rtc::scoped_refptr<AMDContext>(new rtc::RefCountedObject<AMDContext>());
    }

    AMDContext::~AMDContext() {
        g_AMFFactory.Terminate();
    }

    AMFFactoryHelper* GetAMFFactoryHelper(rtc::scoped_refptr<AMDContext>&) {
        return &g_AMFFactory;
    }
} // amd