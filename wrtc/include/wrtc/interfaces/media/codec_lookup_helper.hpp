//
// Created by Laky64 on 25/04/25.
//

#pragma once
#include <pc/codec_vendor.h>

namespace wrtc {

    class CodecLookupHelper final: public cricket::CodecLookupHelper {
        std::unique_ptr<cricket::CodecVendor> codecVendor;
        webrtc::PayloadTypeSuggester *payloadTypeSuggester;

    public:
        CodecLookupHelper(
            cricket::MediaEngineInterface *mediaEngine,
            const cricket::TransportDescriptionFactory *transportDescriptionFactory,
            webrtc::PayloadTypeSuggester *payloadTypeSuggester
        );

        ~CodecLookupHelper() override;

        webrtc::PayloadTypeSuggester* PayloadTypeSuggester() override;

        cricket::CodecVendor* CodecVendor(const std::string& mid) override;
    };

} // wrtc
