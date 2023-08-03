# NativeTgCalls

### FFMPEG COMMAND:
`ffmpeg -i bensound-creativeminds.mp3 -f s16le -ac 1 -ar 48000 -f segment -segment_time 1 opus/sample-%03d.opus`