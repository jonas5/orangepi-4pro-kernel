// Performance and Codec Optimizations for Orange Pi 4 Pro (A733 / PowerVR Rogue)
pref("gfx.webrender.all", true);
pref("widget.wayland.opaque-region.enabled", true);

// Force YouTube to serve smooth H.264 (avc1) instead of CPU-heavy VP9 or AV1
pref("media.av1.enabled", false);
pref("media.mediasource.vp9.enabled", false);
pref("media.webm.enabled", false);

// Prefer system FFmpeg with ARM NEON assembly
pref("media.ffmpeg.enabled", true);
pref("media.ffvpx.enabled", false);

// Optimize video decoder pipeline
pref("media.hardware-video-decoding.enabled", false);
pref("media.navigator.mediadatadecoder_vpx_enabled", false);
