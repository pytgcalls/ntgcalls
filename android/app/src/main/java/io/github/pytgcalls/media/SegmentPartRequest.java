package io.github.pytgcalls.media;

public class SegmentPartRequest {
    public final long segmentId;
    public final int partId;
    public final int limit;
    public final long timestamp;
    public final boolean qualityUpdate;
    public final int channelId;
    public final MediaSegmentQuality quality;

    public SegmentPartRequest(long segmentId, int partId, int limit, long timestamp, boolean qualityUpdate, int channelId, MediaSegmentQuality quality) {
        this.segmentId = segmentId;
        this.partId = partId;
        this.limit = limit;
        this.timestamp = timestamp;
        this.qualityUpdate = qualityUpdate;
        this.channelId = channelId;
        this.quality = quality;
    }
}
