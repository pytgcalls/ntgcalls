//
// Created by Lauren on 25/04/25.
//

#pragma once
#include <pc/codec_vendor.h>

namespace wrtc::interfaces::media {

    class CodecLookupHelper final: public webrtc::CodecLookupHelper {
        std::unique_ptr<webrtc::CodecVendor> codec_vendor_;
        webrtc::PayloadTypeSuggester* payload_type_suggester_;

    public:
        CodecLookupHelper(
            webrtc::MediaEngineInterface* media_engine,
            const webrtc::TransportDescriptionFactory* transport_description_factory,
            webrtc::PayloadTypeSuggester* payload_type_suggester
        );

        ~CodecLookupHelper() override;

        webrtc::PayloadTypeSuggester* PayloadTypeSuggester() override;

        webrtc::CodecVendor* GetCodecVendor() override;
    };

} // wrtc::interfaces::media
