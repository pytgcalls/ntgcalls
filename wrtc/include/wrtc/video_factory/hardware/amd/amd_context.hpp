//
// Created by Laky64 on 10/04/25.
//

#pragma once

#include <mutex>
#include <rtc_base/ref_counted_object.h>

namespace amd {

    class AMDContext : public webrtc::RefCountInterface {

        static std::mutex _mutex;
        static bool initialized;
        static rtc::scoped_refptr<AMDContext> _default;

    public:
        static rtc::scoped_refptr<AMDContext> GetOrCreateDefault();

        ~AMDContext() override;
    };

    AMFFactoryHelper* GetAMFFactoryHelper(rtc::scoped_refptr<AMDContext>&);

} // amd
