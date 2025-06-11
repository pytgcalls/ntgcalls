//
// Created by Laky64 on 14/04/25.
//

#include <bitset>
#include <rtc_base/checks.h>
#include <rtc_base/base64.h>
#include <wrtc/interfaces/mtproto/audio_streaming_part_internal.hpp>

namespace wrtc {
    AudioStreamingPartInternal::AudioStreamingPartInternal(bytes::binary &&data, const std::string& container) {
        avIoContext = std::make_unique<AVIOContextImpl>(std::move(data));
        frame = av_frame_alloc();

        const AVInputFormat *inputFormat = av_find_input_format(container.c_str());
        if (!inputFormat) {
            didReadToEnd = true;
            return;
        }

        inputFormatContext = avformat_alloc_context();
        if (!inputFormatContext) {
            didReadToEnd = true;
            return;
        }

        inputFormatContext->pb = avIoContext->getContext();

        if (avformat_open_input(&inputFormatContext, "", inputFormat, nullptr) < 0) {
            didReadToEnd = true;
            return;
        }

        if (avformat_find_stream_info(inputFormatContext, nullptr) < 0) {
            didReadToEnd = true;

            avformat_close_input(&inputFormatContext);
            inputFormatContext = nullptr;
            return;
        }

        for (int i = 0; i < inputFormatContext->nb_streams; i++) {
            AVStream *inStream = inputFormatContext->streams[i];

            AVCodecParameters *inCodecpar = inStream->codecpar;
            if (inCodecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
                continue;
            }

            audioCodecParameters = avcodec_parameters_alloc();
            avcodec_parameters_copy(audioCodecParameters, inCodecpar);

            streamId = i;
            durationInMilliseconds = static_cast<int>(static_cast<double>(inStream->duration) * av_q2d(inStream->time_base) * 1000);

            if (inStream->metadata) {
                AVDictionaryEntry *entry = av_dict_get(inStream->metadata, "TG_META", nullptr, 0);
                if (entry && entry->value) {
                    if (std::optional<std::string> result = webrtc::Base64Decode(entry->value, webrtc::Base64DecodeOptions::kForgiving); result.has_value()) {
                        int offset = 0;
                        channelUpdates = parseChannelUpdates(result.value(), offset);
                    }
                }

                uint32_t videoChannelMask = 0;
                entry = av_dict_get(inStream->metadata, "ACTIVE_MASK", nullptr, 0);
                if (entry && entry->value) {
                    std::string sourceString = entry->value;
                    videoChannelMask = stringToUInt32(sourceString);
                }

                std::vector<std::string> endpointList;
                entry = av_dict_get(inStream->metadata, "ENDPOINTS", nullptr, 0);
                if (entry && entry->value) {
                    std::string sourceString = entry->value;
                    std::vector<std::string> elems;
                    splitString(sourceString, ' ', std::back_inserter(elems));
                    endpointList = elems;
                }

                if (std::bitset<32> videoChannels(videoChannelMask); videoChannels.count() == endpointList.size()) {
                    size_t endpointIndex = 0;
                    for (size_t i2 = 0; i2 < videoChannels.size(); i2++) {
                        if (videoChannels[i2]) {
                            endpointMapping.insert(std::make_pair(endpointList[endpointIndex], static_cast<int32_t>(i2)));
                            endpointIndex++;
                        }
                    }
                }
            }
            break;
        }

        if (streamId == -1) {
            didReadToEnd = true;
        }
    }

    AudioStreamingPartInternal::~AudioStreamingPartInternal() {
        if (frame) {
            av_frame_free(&frame);
        }
        if (inputFormatContext) {
            avformat_close_input(&inputFormatContext);
        }
        if (audioCodecParameters) {
            avcodec_parameters_free(&audioCodecParameters);
        }
        avIoContext = nullptr;
    }

    std::map<std::string, int32_t> AudioStreamingPartInternal::getEndpointMapping() const {
        return endpointMapping;
    }

    std::vector<AudioStreamingPartInternal::ChannelUpdate> AudioStreamingPartInternal::getChannelUpdates() const {
        return channelUpdates;
    }

    int AudioStreamingPartInternal::getDurationInMilliseconds() const {
        return durationInMilliseconds;
    }

    uint32_t AudioStreamingPartInternal::stringToUInt32(const std::string& string) {
        std::stringstream stringStream(string);
        uint32_t value = 0;
        stringStream >> value;
        return value;
    }

    std::optional<uint32_t> AudioStreamingPartInternal::readInt32(const std::string& data, int& offset) {
        if (offset + 4 > data.length()) {
            return std::nullopt;
        }

        int32_t value = 0;
        memcpy(&value, data.data() + offset, 4);
        offset += 4;

        return value;
    }

    std::vector<AudioStreamingPartInternal::ChannelUpdate> AudioStreamingPartInternal::parseChannelUpdates(const std::string& data, int& offset) {
        std::vector<ChannelUpdate> result;

        if (!readInt32(data, offset)) {
            return {};
        }

        const auto count = readInt32(data, offset);
        if (!count) {
            return {};
        }

        for (int i = 0; i < count.value(); i++) {
            auto frameIndex = readInt32(data, offset);
            if (!frameIndex) {
                return {};
            }

            auto channelId = readInt32(data, offset);
            if (!channelId) {
                return {};
            }

            auto ssrc = readInt32(data, offset);
            if (!ssrc) {
                return {};
            }

            ChannelUpdate update;
            update.frameIndex = static_cast<int>(frameIndex.value());
            update.id = static_cast<int>(channelId.value());
            update.ssrc = ssrc.value();
            result.push_back(update);
        }

        return result;
    }

    int16_t AudioStreamingPartInternal::sampleFloatToInt16(const float sample) {
        return av_clip_int16(static_cast<int32_t>(lrint(sample*32767)));
    }

    AudioStreamingPartInternal::ReadPcmResult AudioStreamingPartInternal::readPcm(AudioStreamingPartPersistentDecoder& persistentDecoder, std::vector<int16_t>& outPcm) {
        if (didReadToEnd) {
            return {};
        }

        int outPcmSampleOffset = 0;
        ReadPcmResult result;

        if (pcmBufferSampleOffset >= pcmBufferSampleSize) {
            fillPcmBuffer(persistentDecoder);
        }

        if (outPcm.size() != 480 * channelCount) {
            outPcm.resize(480 * channelCount);
        }
        int readSamples = 0;
        if (channelCount != 0) {
            readSamples = static_cast<int>(outPcm.size()) / channelCount;
        }

        while (outPcmSampleOffset < readSamples) {
            if (pcmBufferSampleOffset >= pcmBufferSampleSize) {
                fillPcmBuffer(persistentDecoder);

                if (pcmBufferSampleOffset >= pcmBufferSampleSize) {
                    break;
                }
            }

            if (const int readFromPcmBufferSamples = std::min(pcmBufferSampleSize - pcmBufferSampleOffset, readSamples - outPcmSampleOffset); readFromPcmBufferSamples != 0) {
                std::copy_n(pcmBuffer.begin() + pcmBufferSampleOffset * channelCount, readFromPcmBufferSamples * channelCount, outPcm.begin() + outPcmSampleOffset * channelCount);
                pcmBufferSampleOffset += readFromPcmBufferSamples;
                outPcmSampleOffset += readFromPcmBufferSamples;
                result.numSamples += readFromPcmBufferSamples;
                readSampleCount += readFromPcmBufferSamples;
            }
        }

        result.numChannels = channelCount;

        return result;
    }

    void AudioStreamingPartInternal::fillPcmBuffer(AudioStreamingPartPersistentDecoder& persistentDecoder) {
        pcmBufferSampleSize = 0;
        pcmBufferSampleOffset = 0;

        if (didReadToEnd) {
            return;
        }
        if (!inputFormatContext) {
            didReadToEnd = true;
            return;
        }

        int ret = 0;
        while (true) {
            ret = av_read_frame(inputFormatContext, &packet);
            if (ret < 0) {
                didReadToEnd = true;
                return;
            }

            if (packet.stream_index != streamId) {
                av_packet_unref(&packet);
                continue;
            }

            ret = persistentDecoder.decode(audioCodecParameters, inputFormatContext->streams[streamId]->time_base, packet, frame);
            av_packet_unref(&packet);

            if (ret == AVERROR(EAGAIN)) {
                continue;
            }

            break;
        }

        if (ret != 0) {
            didReadToEnd = true;
            return;
        }

        if (channelCount == 0) {
            channelCount = frame->ch_layout.nb_channels;
        }

        if (channelCount == 0) {
            didReadToEnd = true;
            return;
        }

        if (frame->ch_layout.nb_channels != channelCount || frame->ch_layout.nb_channels > 8) {
            didReadToEnd = true;
            return;
        }

        if (pcmBuffer.size() < frame->nb_samples * frame->ch_layout.nb_channels) {
            pcmBuffer.resize(frame->nb_samples * frame->ch_layout.nb_channels);
        }

        switch (frame->format) {
            case AV_SAMPLE_FMT_S16: {
                memcpy(pcmBuffer.data(), frame->data[0], frame->nb_samples * 2 * frame->ch_layout.nb_channels);
            } break;
            case AV_SAMPLE_FMT_S16P: {
                int16_t *toPcm = pcmBuffer.data();
                for (int sample = 0; sample < frame->nb_samples; ++sample) {
                    for (int channel = 0; channel < frame->ch_layout.nb_channels; ++channel) {
                        const auto *shortChannel = reinterpret_cast<int16_t*>(frame->data[channel]);
                        *toPcm++ = shortChannel[sample];
                    }
                }
            } break;
            case AV_SAMPLE_FMT_FLT: {
                const auto *floatData = reinterpret_cast<float*>(&frame->data[0]);
                for (int i = 0; i < frame->nb_samples * frame->ch_layout.nb_channels; i++) {
                    pcmBuffer[i] = sampleFloatToInt16(floatData[i]);
                }
            } break;
            case AV_SAMPLE_FMT_FLTP: {
                int16_t *toPcm2 = pcmBuffer.data();
                for (int sample = 0; sample < frame->nb_samples; ++sample) {
                    for (int channel = 0; channel < frame->ch_layout.nb_channels; ++channel) {
                        const auto *floatChannel = reinterpret_cast<float*>(frame->data[channel]);
                        *toPcm2++ = sampleFloatToInt16(floatChannel[sample]);
                    }
                }
            } break;
            default: {
                RTC_FATAL() << "Unexpected sample_fmt";
            }
        }

        pcmBufferSampleSize = frame->nb_samples;
        pcmBufferSampleOffset = 0;
    }
} // wrtc