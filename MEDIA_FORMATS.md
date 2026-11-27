# Adding codecs and containers

NTgCalls reads and writes every container through **libav**, the static FFmpeg build linked into the library. Raw `PCM` and `I420` are the only formats handled without it.

Most additions need two things: enabling the piece in FFmpeg, and adding a line to a table.

## Where things live

| Path | What it does |
|---|---|
| `media/containers/libav/libav_demuxer` | Reads any container. Opus is passed through, other audio is decoded and re-encoded to Opus, video is passed through |
| `media/containers/libav/libav_muxer` | Writes the container chosen by its `Format` template argument |
| `media/containers/libav/libav_format` | Maps each `Format` to its libav name and file extensions |
| `media/containers/bitstream` | H.264 and AV1 helpers (parameter sets, sequence header) |
| `media/codecs/` | Shared audio pieces: the Opus encoder and the resampler |
| `media/mixers/` | Decide whether a recording gets Opus, PCM or video packets, based on what the muxer supports |
| `media/media_source_factory.cpp` | Picks the reader and writer for a `Format` |

The FFmpeg build flags live in [pytgcalls/ffmpeg](https://github.com/pytgcalls/ffmpeg), in `build.sh`. A pull request that needs a new decoder, encoder, demuxer or muxer goes there first; NTgCalls then bumps `version.ffmpeg` in `version.properties`.

## Reading a new audio codec

Enable the decoder in FFmpeg. Nothing else is needed: anything that isn't Opus is already decoded, resampled and re-encoded.

```sh
--enable-decoder=alac \
```

## Writing a new audio codec

Containers that don't store Opus receive PCM from the mixer and encode it with the codec returned by `pcm_encoder()` in `libav_muxer.hpp`:

```cpp
static constexpr AVCodecID pcm_encoder() {
    switch (Format) {
    case BaseMediaDescription::Format::MP4:
        return AV_CODEC_ID_AAC;
    case BaseMediaDescription::Format::FLAC:
        return AV_CODEC_ID_FLAC;
    ...
```

Add the container to that switch, make `supports(Codec)` return `true` for `Codec::PCM` on it, and enable the encoder in FFmpeg. The encoder picks its own sample format.

## Adding a video codec

WebRTC has to be able to send it, so it must exist in `wrtc::models::Codec`. Then:

1. map it in `LibAvDemuxer::video_codec()` to read it;
2. map it in `LibAvMuxer::codec_id()` and allow it in `supports(Codec)` for the containers that can hold it;
3. add it to `VideoMixer::encoding_codec()` if recordings may re-encode to it;
4. extend `Bitstream` only if the codec needs data taken from the stream, like H.264 SPS/PPS or the AV1 sequence header.

## Adding a container

Using `MOV` as an example:

1. add `MOV` to the `Format` enum in `media_description.hpp`. The bindings generator exposes it to every language;
2. add its libav names to `input_name()` and `output_name()`, and its extensions to `from_path()`, in `libav_format.cpp`;
3. say what it can hold in `supports(MediaKind)` and `supports(Codec)` in `libav_muxer.hpp`;
4. add `template class LibAvMuxer<BaseMediaDescription::Format::MOV>;` at the end of `libav_muxer.cpp`;
5. add a `case` to `create_mixer_writer()` in `media_source_factory.cpp`;
6. enable its demuxer and muxer in FFmpeg.

Pairing a container with the wrong kind of media, like video in WAV, doesn't compile, and users get `InvalidParams` at runtime.

## Code style

- One class or struct per file: header in `include/ntgcalls/...`, implementation in `src/...`.
- No comments in the code, only the file banner and the closing namespace marker.
- No nested lambdas: helpers become private methods, static when possible.
- No duplicated logic: anything needed in two places goes into a shared class.
- Throw `FFmpegError` for unsupported containers or codecs, and `InvalidParams` for invalid combinations.

## Testing

- Run `python -m unittest discover -s tests`.
- Play a file in the new format from `MediaSource.FILE`, and from `MediaSource.SHELL` with a command like `cat file`.
- Record to the new format and check the result with `ffprobe` and `ffmpeg -v error -i output -f null -`, which must print nothing.
