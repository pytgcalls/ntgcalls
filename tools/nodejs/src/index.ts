import {nonstandard, RTCPeerConnection} from 'wrtc';

async function main() {
    const rtc = new RTCPeerConnection();

    rtc.oniceconnectionstatechange = async () => {
        const connection_status = rtc.iceConnectionState;
        if(connection_status){
            console.log(connection_status);
        }
    };

    const audioSource = new nonstandard.RTCAudioSource();
    rtc.addTrack(audioSource.createTrack())

    const offer = await rtc.createOffer({
        offerToReceiveVideo: true,
        offerToReceiveAudio: true,
    });

    const sampleRate = 48000, bitsPerSample = 16, channelCount = 1;

    const bufSize = ((sampleRate * bitsPerSample) / 8 / 100) * channelCount;
    let buffer = new Uint8Array(48000);

    const samples = new Int16Array(new Uint8Array(buffer).buffer);
    audioSource.onData({
        bitsPerSample: bitsPerSample,
        sampleRate: sampleRate,
        channelCount: channelCount,
        numberOfFrames: samples.length,
        buffer,
    });

    if (offer.sdp) {
        console.log(JSON.stringify(parseSdp(offer.sdp)));
    }
}

main().then();

export function parseSdp(sdp: string): Sdp {
    let lines = sdp.split('\r\n');
    let lookup = (prefix: string) => {
        for (let line of lines) {
            if (line.startsWith(prefix)) {
                return line.substring(prefix.length);
            }
        }
        return null;
    };

    let rawAudioSource = lookup('a=ssrc:');
    let rawVideoSource = lookup('a=ssrc-group:FID ');
    return {
        fingerprint: lookup('a=fingerprint:')?.split(' ')[1] ?? null,
        hash: lookup('a=fingerprint:')?.split(' ')[0] ?? null,
        setup: lookup('a=setup:'),
        pwd: lookup('a=ice-pwd:'),
        ufrag: lookup('a=ice-ufrag:'),
        audioSource: rawAudioSource ? parseInt(rawAudioSource.split(' ')[0]) : null,
        source_groups: rawVideoSource ? rawVideoSource.split(' ').map(obj => {
            return parseInt(obj);
        }) : null,
    };
}

export interface Sdp {
    fingerprint: string | null;
    hash: string | null;
    setup: string | null;
    pwd: string | null;
    ufrag: string | null;
    audioSource: number | null;
    source_groups: Array<number> | null;
}