//
// Created by Laky64 on 25/04/25.
//

#pragma once
#include <pc/codec_vendor.h>

namespace wrtc {

    class CodecLookupHelper final: public webrtc::CodecLookupHelper {
        std::unique_ptr<webrtc::CodecVendor> codecVendor;
        webrtc::PayloadTypeSuggester *payloadTypeSuggester;

    public:
        CodecLookupHelper(
            webrtc::MediaEngineInterface *mediaEngine,
            const webrtc::TransportDescriptionFactory *transportDescriptionFactory,
            webrtc::PayloadTypeSuggester *payloadTypeSuggester
        );

        ~CodecLookupHelper() override;

        webrtc::PayloadTypeSuggester* PayloadTypeSuggester() override;

        webrtc::CodecVendor* GetCodecVendor() override;
    };

} // wrtc
