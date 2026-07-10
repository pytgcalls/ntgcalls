//
// Created by Lauren on 25/04/25.
//

#include <wrtc/interfaces/media/codec_lookup_helper.hpp>

namespace wrtc::interfaces::media {
    CodecLookupHelper::CodecLookupHelper(
        webrtc::MediaEngineInterface* media_engine,
        const webrtc::TransportDescriptionFactory* transport_description_factory,
        webrtc::PayloadTypeSuggester* payload_type_suggester
    ) : payload_type_suggester_(payload_type_suggester) {
        codec_vendor_ = std::make_unique<webrtc::CodecVendor>(
            media_engine,
            true,
            transport_description_factory->trials()
        );
    }

    CodecLookupHelper::~CodecLookupHelper() {
        codec_vendor_ = nullptr;
        payload_type_suggester_ = nullptr;
    }

    webrtc::PayloadTypeSuggester* CodecLookupHelper::PayloadTypeSuggester() {
        return payload_type_suggester_;
    }

    webrtc::CodecVendor* CodecLookupHelper::GetCodecVendor() {
        return codec_vendor_.get();
    }
} // wrtc::interfaces::media