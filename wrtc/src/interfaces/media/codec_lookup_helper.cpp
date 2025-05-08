//
// Created by Laky64 on 25/04/25.
//

#include <wrtc/interfaces/media/codec_lookup_helper.hpp>

namespace wrtc {
    CodecLookupHelper::CodecLookupHelper(
        cricket::MediaEngineInterface* mediaEngine,
        const cricket::TransportDescriptionFactory *transportDescriptionFactory,
        webrtc::PayloadTypeSuggester *payloadTypeSuggester
    ) : payloadTypeSuggester(payloadTypeSuggester) {
        codecVendor = std::make_unique<cricket::CodecVendor>(
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

    cricket::CodecVendor* CodecLookupHelper::CodecVendor(const std::string& mid) {
        return codecVendor.get();
    }
} // wrtc