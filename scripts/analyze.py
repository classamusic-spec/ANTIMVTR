#!/usr/bin/env python
"""
Analyse a rendered WAV: prints metrics and writes a spectrogram + waveform PNG
next to the file (or to --png). This is how we *see* what the engine sounds like.

Usage: scripts/analyze.py file.wav [--png out.png] [--title "text"] [--fmax 12000]
"""
import argparse, json, sys
import numpy as np
import soundfile as sf

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("wav")
    ap.add_argument("--png")
    ap.add_argument("--title", default="")
    ap.add_argument("--fmax", type=float, default=12000.0)
    ap.add_argument("--no-plot", action="store_true")
    a = ap.parse_args()

    x, sr = sf.read(a.wav, always_2d=True)
    mono = x.mean(axis=1)
    n = len(mono)
    peak = float(np.max(np.abs(x))) if n else 0.0
    rms = float(np.sqrt(np.mean(mono ** 2))) if n else 0.0
    nonfinite = int(np.sum(~np.isfinite(x)))
    dc = float(np.mean(mono)) if n else 0.0

    # STFT
    win = 2048; hop = 512
    if n >= win:
        frames = np.lib.stride_tricks.sliding_window_view(mono, win)[::hop]
        w = np.hanning(win)
        spec = np.abs(np.fft.rfft(frames * w, axis=1)) / (win / 4)
        freqs = np.fft.rfftfreq(win, 1 / sr)
        mags = spec.mean(axis=0)
        centroid = float((mags * freqs).sum() / max(mags.sum(), 1e-12))
        # spectral flatness (0 = tonal, 1 = noise)
        m = mags[1:] + 1e-12
        flatness = float(np.exp(np.mean(np.log(m))) / np.mean(m))
        # top partials
        peaks = []
        for i in range(2, len(mags) - 2):
            if mags[i] > mags[i-1] and mags[i] > mags[i+1] and mags[i] > mags.max() * 0.02:
                peaks.append((freqs[i], 20*np.log10(mags[i] + 1e-12)))
        peaks.sort(key=lambda p: -p[1])
        top = [(round(f,1), round(d,1)) for f, d in peaks[:12]]
        # envelope (dB per 10 ms)
        env_hop = int(sr * 0.01)
        env = [20*np.log10(np.sqrt(np.mean(mono[i:i+env_hop]**2)) + 1e-9) for i in range(0, n - env_hop, env_hop)]
    else:
        centroid = 0.0; flatness = 0.0; top = []; env = []; spec = None; freqs = None

    out = {
        "file": a.wav, "sr": sr, "seconds": n / sr, "peak": peak, "peak_db": 20*np.log10(peak + 1e-12),
        "rms": rms, "rms_db": 20*np.log10(rms + 1e-12), "dc": dc, "nonfinite": nonfinite,
        "centroid_hz": centroid, "flatness": flatness, "top_partials_hz_db": top,
    }
    print(json.dumps(out, indent=1, default=float))

    if a.no_plot or spec is None:
        return
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    png = a.png or a.wav.rsplit(".", 1)[0] + ".png"
    fig, axes = plt.subplots(3, 1, figsize=(12, 9), gridspec_kw={"height_ratios": [1, 2, 1]})
    t = np.arange(n) / sr
    axes[0].plot(t, mono, lw=0.4, color="#4f8dff"); axes[0].set_xlim(0, t[-1]); axes[0].set_ylim(-1, 1)
    axes[0].set_title(a.title or a.wav); axes[0].set_ylabel("amp")
    db = 20*np.log10(spec.T + 1e-9)
    tt = np.arange(spec.shape[0]) * hop / sr
    fmask = freqs <= a.fmax
    axes[1].pcolormesh(tt, freqs[fmask], db[fmask], shading="auto", cmap="magma", vmin=-90, vmax=0)
    axes[1].set_yscale("symlog", linthresh=200); axes[1].set_ylabel("Hz"); axes[1].set_ylim(30, a.fmax)
    axes[2].plot(np.arange(len(env)) * 0.01, env, color="#e455cf"); axes[2].set_ylim(-90, 0); axes[2].set_ylabel("dB"); axes[2].set_xlabel("s")
    axes[2].set_xlim(0, n / sr)
    fig.tight_layout(); fig.savefig(png, dpi=80); print("wrote", png, file=sys.stderr)

if __name__ == "__main__":
    main()
