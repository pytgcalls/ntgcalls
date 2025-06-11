//
// Created by Laky64 on 25/04/25.
//

#include <wrtc/interfaces/media/codec_lookup_helper.hpp>

namespace wrtc {
    CodecLookupHelper::CodecLookupHelper(
        webrtc::MediaEngineInterface* mediaEngine,
        const webrtc::TransportDescriptionFactory *transportDescriptionFactory,
        webrtc::PayloadTypeSuggester *payloadTypeSuggester
    ) : payloadTypeSuggester(payloadTypeSuggester) {
        codecVendor = std::make_unique<webrtc::CodecVendor>(
            mediaEngine,
            true,
            transportDescriptionFactory->trials()
        );
    }

    CodecLookupHelper::~CodecLookupHelper() {
        codecVendor = nullptr;
        payloadTypeSuggester = nullptr;
    }

    webrtc::PayloadTypeSuggester* CodecLookupHelper::PayloadTypeSuggester() {
        return payloadTypeSuggester;
    }

    webrtc::CodecVendor* CodecLookupHelper::GetCodecVendor() {
        return codecVendor.get();
    }
} // wrtc