#!/usr/bin/env python3
import argparse
import csv
import math
import re
from pathlib import Path


def median(values):
    values = sorted(values)
    n = len(values)
    if n == 0:
        return 0.0
    mid = n // 2
    if n % 2:
        return float(values[mid])
    return (values[mid - 1] + values[mid]) / 2.0


def mean(values):
    return sum(values) / len(values) if values else 0.0


def std(values):
    if len(values) < 2:
        return 0.0
    m = mean(values)
    return math.sqrt(sum((x - m) ** 2 for x in values) / (len(values) - 1))


def moving_average(values, window):
    if window <= 1:
        return [float(v) for v in values]
    half = window // 2
    out = []
    for i in range(len(values)):
        lo = max(0, i - half)
        hi = min(len(values), i + half + 1)
        out.append(mean(values[lo:hi]))
    return out


def load_csv(path):
    rows = []
    skipped = 0
    for line in path.read_bytes().decode("utf-8", errors="ignore").splitlines():
        nums = re.findall(r"\d+", line)
        if len(nums) < 2:
            skipped += 1
            continue
        t = int(nums[-2])
        v = int(nums[-1])
        if 1000 <= v <= 3300:
            rows.append((t, v))
        else:
            skipped += 1
    rows.sort(key=lambda x: x[0])
    return rows, skipped


def detect_peaks(times, values, min_rr_ms=500, threshold_offset_mv=35):
    baseline = median(values)
    threshold = baseline + threshold_offset_mv
    candidates = []
    for i in range(1, len(values) - 1):
        if values[i] >= values[i - 1] and values[i] > values[i + 1] and values[i] >= threshold:
            candidates.append((times[i], values[i]))

    peaks = []
    for t, v in candidates:
        if not peaks or t - peaks[-1][0] >= min_rr_ms:
            peaks.append((t, v))
        elif v > peaks[-1][1]:
            peaks[-1] = (t, v)
    return peaks, threshold


def rr_features(peaks):
    rr = [peaks[i][0] - peaks[i - 1][0] for i in range(1, len(peaks))]
    valid_rr = [x for x in rr if 400 <= x <= 1500]
    if not valid_rr:
        return rr, {}

    diff_rr = [valid_rr[i] - valid_rr[i - 1] for i in range(1, len(valid_rr))]
    abs_diff_rr = [abs(x) for x in diff_rr]
    rmssd = math.sqrt(mean([x * x for x in diff_rr])) if diff_rr else 0.0
    pnn50 = 100.0 * sum(1 for x in abs_diff_rr if x > 50) / len(abs_diff_rr) if abs_diff_rr else 0.0

    features = {
        "rr_count": len(valid_rr),
        "rr_mean_ms": mean(valid_rr),
        "rr_median_ms": median(valid_rr),
        "rr_min_ms": min(valid_rr),
        "rr_max_ms": max(valid_rr),
        "rr_std_ms": std(valid_rr),
        "heart_rate_mean_bpm": 60000.0 / mean(valid_rr),
        "heart_rate_median_bpm": 60000.0 / median(valid_rr),
        "sdnn_ms": std(valid_rr),
        "rmssd_ms": rmssd,
        "pnn50_percent": pnn50,
    }
    return rr, features


def simple_frequency_features(values, sample_rate_hz):
    centered = [v - mean(values) for v in values]
    n = len(centered)
    if n < 4 or sample_rate_hz <= 0:
        return {}

    max_k = n // 2
    total_power = 0.0
    low_power = 0.0
    qrs_power = 0.0
    noise_power = 0.0

    for k in range(1, max_k + 1):
        freq = k * sample_rate_hz / n
        re_part = 0.0
        im_part = 0.0
        for i, x in enumerate(centered):
            angle = -2.0 * math.pi * k * i / n
            re_part += x * math.cos(angle)
            im_part += x * math.sin(angle)
        power = (re_part * re_part + im_part * im_part) / n
        total_power += power
        if 0.5 <= freq < 5.0:
            low_power += power
        elif 5.0 <= freq < 15.0:
            qrs_power += power
        elif freq >= 15.0:
            noise_power += power

    return {
        "signal_total_power": total_power,
        "power_0p5_5hz": low_power,
        "power_5_15hz": qrs_power,
        "power_above_15hz": noise_power,
        "qrs_to_low_power_ratio": qrs_power / low_power if low_power > 0 else 0.0,
    }


def write_csv(path, header, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(header)
        writer.writerows(rows)


def plot_middle_segment(out_path, times, raw, smooth, peaks, seconds):
    import matplotlib.pyplot as plt

    duration_ms = times[-1] - times[0]
    window_ms = int(seconds * 1000)
    if duration_ms <= window_ms:
        start_ms = times[0]
        end_ms = times[-1]
    else:
        center_ms = times[0] + duration_ms // 2
        start_ms = center_ms - window_ms // 2
        end_ms = center_ms + window_ms // 2

    seg = [(t, r, s) for t, r, s in zip(times, raw, smooth) if start_ms <= t <= end_ms]
    peak_map = {t: v for t, v in peaks if start_ms <= t <= end_ms}
    if not seg:
        return

    t0 = seg[0][0]
    x = [(t - t0) / 1000.0 for t, _, _ in seg]
    raw_y = [r for _, r, _ in seg]
    smooth_y = [s for _, _, s in seg]
    peak_x = [(t - t0) / 1000.0 for t in peak_map]
    peak_y = [peak_map[t] for t in peak_map]

    plt.figure(figsize=(12, 5))
    plt.plot(x, raw_y, color="#c8d6e5", linewidth=1.0, label="raw adc_output[2]")
    plt.plot(x, smooth_y, color="#ff7f0e", linewidth=1.6, label="filtered ECG")
    if peak_x:
        plt.scatter(peak_x, peak_y, color="#d62728", s=28, zorder=3, label="R peaks")
    plt.xlabel("Time (s)")
    plt.ylabel("Voltage (mV)")
    plt.title(f"Filtered ECG Segment ({seconds:g}s from middle)")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(out_path, dpi=160)
    plt.close()


def main():
    parser = argparse.ArgumentParser(description="Analyze ECG adc_output[2] CSV logs.")
    parser.add_argument("input", nargs="?", default="log/data2_quiet.csv", help="CSV file: timestamp_ms,adc_output_mv")
    parser.add_argument("--out-dir", default="log/processed", help="Output directory")
    parser.add_argument("--smooth-window", type=int, default=3, help="Moving-average window in samples")
    parser.add_argument("--threshold-offset", type=float, default=35.0, help="Peak threshold above median, in mV")
    parser.add_argument("--min-rr-ms", type=int, default=500, help="Minimum interval between R peaks")
    parser.add_argument("--plot-seconds", type=float, default=10.0, help="Seconds to plot from the middle of the record")
    args = parser.parse_args()

    input_path = Path(args.input)
    out_dir = Path(args.out_dir)
    rows, skipped = load_csv(input_path)
    if len(rows) < 10:
        raise SystemExit(f"Not enough valid samples in {input_path}. valid={len(rows)}, skipped={skipped}")

    times = [t for t, _ in rows]
    raw = [v for _, v in rows]
    dts = [times[i] - times[i - 1] for i in range(1, len(times))]
    sample_rate_hz = 1000.0 / median(dts) if dts else 0.0
    smooth = moving_average(raw, args.smooth_window)

    peaks, threshold = detect_peaks(times, smooth, args.min_rr_ms, args.threshold_offset)
    rr, rr_stats = rr_features(peaks)
    freq_stats = simple_frequency_features(smooth, sample_rate_hz)

    peak_times = {t for t, _ in peaks}
    processed_rows = [
        [t, raw_v, round(smooth_v, 3), 1 if t in peak_times else 0]
        for t, raw_v, smooth_v in zip(times, raw, smooth)
    ]

    write_csv(
        out_dir / f"{input_path.stem}_processed.csv",
        ["time_ms", "adc_output_mv", "smoothed_mv", "is_r_peak"],
        processed_rows,
    )
    write_csv(out_dir / f"{input_path.stem}_peaks.csv", ["peak_time_ms", "peak_value_mv"], peaks)
    write_csv(out_dir / f"{input_path.stem}_rr.csv", ["rr_ms"], [[x] for x in rr])
    plot_middle_segment(
        out_dir / f"{input_path.stem}_filtered_10s.png",
        times,
        raw,
        smooth,
        peaks,
        args.plot_seconds,
    )

    summary = {
        "input_file": str(input_path),
        "valid_samples": len(rows),
        "skipped_lines": skipped,
        "duration_s": (times[-1] - times[0]) / 1000.0,
        "dt_median_ms": median(dts),
        "dt_min_ms": min(dts) if dts else 0,
        "dt_max_ms": max(dts) if dts else 0,
        "sample_rate_hz": sample_rate_hz,
        "adc_mean_mv": mean(raw),
        "adc_median_mv": median(raw),
        "adc_std_mv": std(raw),
        "adc_min_mv": min(raw),
        "adc_max_mv": max(raw),
        "peak_threshold_mv": threshold,
        "r_peak_count": len(peaks),
    }
    summary.update(rr_stats)
    summary.update(freq_stats)

    zh_labels = {
        "input_file": "输入文件",
        "valid_samples": "有效样本数",
        "skipped_lines": "跳过的异常行数",
        "duration_s": "记录时长",
        "dt_median_ms": "中位时间间隔",
        "dt_min_ms": "最小时间间隔",
        "dt_max_ms": "最大时间间隔",
        "sample_rate_hz": "采样率",
        "adc_mean_mv": "ADC平均电压",
        "adc_median_mv": "ADC中位电压",
        "adc_std_mv": "ADC电压标准差",
        "adc_min_mv": "ADC最小电压",
        "adc_max_mv": "ADC最大电压",
        "peak_threshold_mv": "R峰检测阈值",
        "r_peak_count": "R峰数量",
        "rr_count": "RR间期数量",
        "rr_mean_ms": "RR平均间期",
        "rr_median_ms": "RR中位间期",
        "rr_min_ms": "RR最小间期",
        "rr_max_ms": "RR最大间期",
        "rr_std_ms": "RR间期标准差",
        "heart_rate_mean_bpm": "平均心率",
        "heart_rate_median_bpm": "中位心率",
        "sdnn_ms": "SDNN心率变异性",
        "rmssd_ms": "RMSSD相邻RR差均方根",
        "pnn50_percent": "pNN50相邻RR差超过50ms比例",
        "signal_total_power": "信号总功率",
        "power_0p5_5hz": "0.5到5Hz频段功率",
        "power_5_15hz": "5到15Hz频段功率",
        "power_above_15hz": "15Hz以上频段功率",
        "qrs_to_low_power_ratio": "QRS频段与低频功率比",
    }

    with (out_dir / f"{input_path.stem}_summary.txt").open("w", encoding="utf-8-sig") as f:
        for key, value in summary.items():
            label = f"{key}/{zh_labels[key]}" if key in zh_labels else key
            if isinstance(value, float):
                f.write(f"{label}: {value:.4f}\n")
            else:
                f.write(f"{label}: {value}\n")

    print("Analysis complete.")
    print(f"Input: {input_path}")
    print(f"Valid samples: {len(rows)}, duration: {summary['duration_s']:.2f}s, sample rate: {sample_rate_hz:.2f}Hz")
    print(f"Detected R peaks: {len(peaks)}")
    if rr_stats:
        print(f"Heart rate mean: {rr_stats['heart_rate_mean_bpm']:.1f} bpm")
        print(f"Heart rate median: {rr_stats['heart_rate_median_bpm']:.1f} bpm")
        print(f"RR mean: {rr_stats['rr_mean_ms']:.1f} ms, SDNN: {rr_stats['sdnn_ms']:.1f} ms, RMSSD: {rr_stats['rmssd_ms']:.1f} ms")
    else:
        print("RR features unavailable: not enough valid R peaks.")
    print(f"Outputs written to: {out_dir}")
    print(f"Plot written to: {out_dir / f'{input_path.stem}_filtered_10s.png'}")


if __name__ == "__main__":
    main()
