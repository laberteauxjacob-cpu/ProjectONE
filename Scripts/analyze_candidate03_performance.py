#!/usr/bin/env python3
"""Analyze a completed Unreal CSV profiler capture; standard library only.

Timings are milliseconds, not FPS or seconds. UE's final header is authoritative:
new scope columns can appear after the initial header. Nested/worker physics
scopes are reported individually, never added into a fictitious wall-time total.
Raw command lines, login/device identifiers and event text are not published.
"""

from __future__ import annotations

import argparse
import csv
import gzip
import hashlib
import json
import math
import re
import statistics
import sys
from pathlib import Path


SPIKES_MS = (16.7, 33.3, 50.0, 100.0)
PHYSICS_CATEGORIES = {"physics", "physicsverbose", "chaos", "chaosphysicssolver", "chaosphysicstimers"}
SAFE_METADATA = {
    "platform", "config", "buildversion", "engineversion", "enginereleaseversion",
    "os", "cpu", "gpu", "deviceprofile", "systemresolution.resx",
    "systemresolution.resy", "rhi", "one_source_commit", "one_scenario",
    "one_enemies_requested", "one_enemies_observed", "one_resolution",
    "one_lighting", "one_recording", "one_max_fps", "one_vsync",
}
SETTINGS = {
    "r.VSync", "t.MaxFPS", "r.ScreenPercentage", "r.AntiAliasingMethod",
    "r.DynamicGlobalIlluminationMethod", "r.ReflectionMethod",
    "sg.ViewDistanceQuality", "sg.AntiAliasingQuality", "sg.ShadowQuality",
    "sg.PostProcessQuality", "sg.TextureQuality", "sg.EffectsQuality",
    "sg.FoliageQuality", "sg.ShadingQuality", "csv.AggregateTaskWorkerStats",
}
SETTING_CASE = {name.lower(): name for name in SETTINGS}
PRIVATE_VALUE = re.compile(r"(?:[A-Za-z]:[\\/]|\\\\|/(?:home|Users)/|"
                           r"gh[pousr]_[A-Za-z0-9]{20,}|github_pat_|-----BEGIN )", re.I)


def safe_scalar(value: str) -> bool:
    return len(value) <= 240 and not PRIVATE_VALUE.search(value) and not any(
        ord(c) < 32 for c in value
    )


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_capture(path: Path, allow_incomplete: bool = False) -> dict:
    opener = gzip.open if path.suffix.lower() == ".gz" else open
    headers, rows, metadata = [], [], {}
    with opener(path, "rt", encoding="utf-8-sig", newline="") as stream:
        for line_no, row in enumerate(csv.reader(stream), 1):
            if not row or all(not cell.strip() for cell in row):
                continue
            first = row[0].strip()
            if first.upper() == "EVENTS":
                header = [cell.strip() for cell in row]
                if headers and header[:len(headers[-1])] != headers[-1]:
                    raise ValueError("Final CSV header does not extend the initial header")
                headers.append(header)
            elif first.startswith("["):
                # UE writes [Key],Value pairs on one final line. Commandline is
                # last and may contain commas; it is deliberately never retained.
                i = 0
                while i + 1 < len(row):
                    match = re.fullmatch(r"\[([^\]]+)\]", row[i].strip())
                    if not match:
                        break
                    key = match.group(1).lower()
                    if key == "commandline":
                        break
                    metadata[key] = row[i + 1].strip()
                    i += 2
            elif headers:
                rows.append((line_no, row))
            else:
                raise ValueError("Not an Unreal CSV profiler capture (EVENTS header absent)")
    if not headers or not rows:
        raise ValueError("Capture contains no frame rows")
    complete = len(headers) >= 2 and metadata.get("hasheaderrowatend") == "1"
    if not complete and not allow_incomplete:
        raise ValueError("Capture lacks the final header/footer; stop CSV capture and wait for writing to finish")
    header = headers[-1]
    for line_no, row in rows:
        if len(row) > len(header):
            raise ValueError(f"Too many columns at line {line_no}; capture is malformed")
    safe = {k: v for k, v in metadata.items() if k in SAFE_METADATA and safe_scalar(v)}
    return {"header": header, "rows": rows, "complete": complete,
            "metadata": safe,
            "metadata_fields_omitted": sum(k not in safe and k != "hasheaderrowatend" for k in metadata)}


def percentile(sorted_values: list[float], p: float) -> float:
    """Linear interpolation at (n-1)*p, explicitly shared by every metric."""
    location = (len(sorted_values) - 1) * p
    lower = math.floor(location)
    upper = math.ceil(location)
    return sorted_values[lower] + (sorted_values[upper] - sorted_values[lower]) * (location - lower)


def summary(values: list[float]) -> dict:
    ordered = sorted(values)
    return {
        "samples": len(values), "mean_ms": statistics.fmean(values),
        "median_ms": percentile(ordered, .5), "p95_ms": percentile(ordered, .95),
        "p99_ms": percentile(ordered, .99), "max_ms": ordered[-1],
        "nonzero_samples": sum(v > 0 for v in values),
        "spikes": {f"above_{limit:g}_ms": {"count": sum(v > limit for v in values),
                    "percent": 100 * sum(v > limit for v in values) / len(values)} for limit in SPIKES_MS},
    }


def numeric(row: list[str], index: int, line_no: int, column: str) -> float:
    # Earlier UE rows omit columns whose series was first registered later.
    # Unreal's stream writer treats these not-yet-executed series as zero.
    text = row[index].strip() if index < len(row) else ""
    try:
        value = float(text) if text else 0.0
    except ValueError as error:
        raise ValueError(f"Non-numeric timing in column {column!r} at line {line_no}") from error
    if not math.isfinite(value) or value < 0:
        raise ValueError(f"Invalid timing in column {column!r} at line {line_no}")
    return value


def is_physics_scope(name: str) -> bool:
    parts = name.split("/")
    # Default UE timing names: Category/Thread/Scope; custom counters lack the
    # thread segment. COUNTS/Category/Thread/Scope counts calls, never ms.
    return len(parts) >= 3 and parts[0].lower() in PHYSICS_CATEGORIES


def parse_interval(text: str) -> tuple[float, float]:
    try:
        start, end = (float(part) for part in text.split(":"))
    except ValueError as error:
        raise argparse.ArgumentTypeError("Expected START:END in seconds") from error
    if not all(math.isfinite(v) for v in (start, end)) or start < 0 or end <= start:
        raise argparse.ArgumentTypeError("Interval must satisfy 0 <= START < END")
    return start, end


def benchmark_metadata(path: Path | None) -> dict:
    """Read only allowlisted values from the existing ONEValidation report."""
    if path is None:
        return {}
    result = {}
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        match = re.fullmatch(r"([\w.]+)=([-+\d.eE]+)", line.strip())
        if match and match.group(1).lower() in SETTING_CASE:
            value = float(match.group(2))
            if math.isfinite(value):
                result[SETTING_CASE[match.group(1).lower()]] = value
        elif line.startswith(("GPU: ", "Resolution: ")):
            key, value = line.split(": ", 1)
            if safe_scalar(value):
                result[key.lower()] = value
        match = re.search(r"Sustained runtime actual alive=(\d+) requested=(\d+)", line)
        if match:
            result["enemies_observed_at_end"] = int(match.group(1))
            result["enemies_requested"] = int(match.group(2))
    return result


def analyze(args: argparse.Namespace) -> tuple[dict, list[dict]]:
    capture = read_capture(args.input, args.allow_incomplete)
    header = capture["header"]
    if args.frame_column not in header:
        raise ValueError(f"Frame timing column {args.frame_column!r} absent; screenshot manifests are not profiler timings")
    physics = [name for name in header if is_physics_scope(name)]
    for name in args.physics_column:
        if name not in header or name.upper().startswith(("COUNTS/", "PHYSICSCOUNTERS/")):
            raise ValueError("Explicit physics timing column absent or is a call counter")
        if name not in physics:
            physics.append(name)
    threads = [name for name in header if name in {
        "GameThread", "RenderThread", "RHIThread", "GPUFrameTime",
        "GameThreadTime", "RenderThreadTime", "RHIThreadTime",
    }]
    counters = [name for name in header if name.lower().startswith("physicscounters/") or (
        name.upper().startswith("COUNTS/") and any(p.lower() in PHYSICS_CATEGORIES for p in name.split("/")))]
    metric_names = list(dict.fromkeys([args.frame_column] + threads + physics))
    for name in metric_names + counters:
        if header.count(name) != 1:
            raise ValueError(f"Ambiguous duplicate selected metric column: {name!r}")
    indices = {name: header.index(name) for name in metric_names}
    elapsed = 0.0
    selected, zero_frames, excluded_frames = [], 0, 0
    for frame_index, (line_no, row) in enumerate(capture["rows"]):
        frame_ms = numeric(row, indices[args.frame_column], line_no, args.frame_column)
        start, end = elapsed, elapsed + frame_ms / 1000
        elapsed = end
        if frame_ms == 0:
            zero_frames += 1
            continue
        # Select complete frames only, and exclude the whole frame if it touches
        # an excluded time interval. This never clips a long frame into a short one.
        if start < args.start_seconds or (args.end_seconds is not None and end > args.end_seconds) or any(
            start < b and end > a for a, b in args.exclude_seconds
        ):
            excluded_frames += 1
            continue
        selected.append({"capture_frame_index": frame_index, "elapsed_start_seconds": start,
                         "event_count": len([e for e in row[0].split(";") if e]),
                         "timings_ms": {name: numeric(row, i, line_no, name) for name, i in indices.items()},
                         "native_counters": {name: numeric(row, header.index(name), line_no, name) for name in counters}})
    if not selected:
        raise ValueError("No positive-duration frames remain in the selected interval")
    metrics = {name: summary([row["timings_ms"][name] for row in selected]) for name in metric_names}
    counter_metrics = {}
    for name in counters:
        values = [row["native_counters"][name] for row in selected]
        counter_metrics[name] = {"unit": "scope_calls" if name.upper().startswith("COUNTS/") else "native_counter_not_time",
                                 "samples": len(values), "mean": statistics.fmean(values),
                                 "max": max(values), "last": values[-1]}
    operator = {key: value for key, value in {
        "scenario": args.scenario, "source_commit": args.source_commit,
        "enemies_requested": args.enemy_count, "enemies_observed": args.observed_enemy_count,
        "resolution": args.resolution, "max_fps": args.max_fps, "vsync": args.vsync,
        "recording_enabled": args.recording,
    }.items() if value is not None}
    for value in operator.values():
        if isinstance(value, str) and not safe_scalar(value):
            raise ValueError("Operator metadata contains a path or unsupported free-text value")
    log_metadata = benchmark_metadata(args.settings_log)
    warnings = []
    if not capture["complete"]:
        warnings.append("Incomplete capture explicitly allowed; missing late columns/footer may hide metrics.")
    if not physics:
        warnings.append("Physics CPU scopes unavailable. Enable Chaos/PhysicsVerbose and verify captured headers; do not infer cost from frame time.")
    elif not any(metrics[name]["nonzero_samples"] for name in physics):
        warnings.append("Physics scope columns exist but are all zero in this interval; no measurable activity was recorded.")
    if args.enemy_count is None and "one_enemies_requested" not in capture["metadata"] and "enemies_requested" not in log_metadata:
        warnings.append("Requested enemy count was not supplied in capture/operator metadata.")
    report = {
        "schema": "projectone.csv_performance.v1", "input": {
            "filename": args.input.name, "sha256": file_sha256(args.input),
            "bytes": args.input.stat().st_size, "completed_capture": capture["complete"],
        },
        "method": {
            "units": "milliseconds", "percentile": "linear interpolation at (n-1)*p",
            "spike_comparison": "strictly greater than threshold",
            "elapsed_clock": "cumulative CSV FrameTime, relative to capture start, not world/actor time",
            "physics_interpretation": "individual CPU scope durations; nested scopes and task-worker aggregates overlap and MUST NOT be summed as wall time",
            "privacy": "allowlisted footer fields only; command line, device/login IDs and event text omitted",
            "late_scope_columns": "use final header; earlier absent values are zero, matching UE writer semantics",
        },
        "capture_metadata": capture["metadata"], "metadata_fields_omitted": capture["metadata_fields_omitted"], "operator_metadata": operator,
        "duplicate_unselected_columns": {name: [i for i, other in enumerate(header) if other == name]
                                         for name in dict.fromkeys(header) if header.count(name) > 1},
        "benchmark_log_metadata": log_metadata,
        "selection": {"start_seconds": args.start_seconds, "end_seconds": args.end_seconds,
                      "excluded_intervals_seconds": args.exclude_seconds,
                      "capture_duration_seconds": elapsed, "capture_rows": len(capture["rows"]),
                      "selected_frames": len(selected), "zero_duration_rows": zero_frames,
                      "excluded_positive_frames": excluded_frames,
                      "selected_duration_seconds": sum(r["timings_ms"][args.frame_column] for r in selected) / 1000},
        "frame_time": {"column": args.frame_column, **metrics[args.frame_column]},
        "thread_timings": {name: metrics[name] for name in threads},
        "physics": {"status": "available" if physics else "unavailable",
                    "scopes": {name: metrics[name] for name in physics},
                    "counters": counter_metrics},
        "worst_frames": sorted(selected, key=lambda r: r["timings_ms"][args.frame_column], reverse=True)[:20],
        "warnings": warnings,
    }
    return report, selected


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="UE CSV profiler .csv or .csv.gz")
    parser.add_argument("--output", type=Path, help="JSON report; defaults to stdout")
    parser.add_argument("--timeline", type=Path, help="Optional numeric per-frame CSV; original frame indices preserved")
    parser.add_argument("--frame-column", default="FrameTime")
    parser.add_argument("--physics-column", action="append", default=[], help="Exact additional known millisecond CPU scope; repeatable")
    parser.add_argument("--start-seconds", type=float, default=0)
    parser.add_argument("--end-seconds", type=float)
    parser.add_argument("--exclude-seconds", action="append", type=parse_interval, default=[])
    parser.add_argument("--allow-incomplete", action="store_true")
    parser.add_argument("--settings-log", type=Path, help="Optional actual ONEValidation benchmark report; only safe settings/counts extracted")
    parser.add_argument("--scenario")
    parser.add_argument("--source-commit")
    parser.add_argument("--enemy-count", type=int, help="Requested count, not proof of observed count")
    parser.add_argument("--observed-enemy-count", type=int, help="Separately verified observed count")
    parser.add_argument("--resolution", help="Example: 1600x900")
    parser.add_argument("--max-fps", type=float, help="0 means uncapped; omit if unknown")
    parser.add_argument("--vsync", choices=("0", "1"))
    parser.add_argument("--recording", choices=("yes", "no", "unknown"))
    args = parser.parse_args()
    try:
        for value in (args.start_seconds, args.end_seconds, args.max_fps, args.enemy_count, args.observed_enemy_count):
            if value is not None and (not math.isfinite(value) or value < 0):
                raise ValueError("Time/count/cap values must be finite and nonnegative")
        if args.end_seconds is not None and args.end_seconds <= args.start_seconds:
            raise ValueError("End time must exceed start time")
        if args.source_commit and not re.fullmatch(r"[0-9a-fA-F]{7,40}", args.source_commit):
            raise ValueError("Source commit must be a 7–40 digit hexadecimal Git revision")
        for output in (args.output, args.timeline):
            if output and output.resolve() in {p.resolve() for p in (args.input, args.settings_log) if p}:
                raise ValueError("Output must not overwrite an input capture/report")
        if args.output and args.timeline and args.output.resolve() == args.timeline.resolve():
            raise ValueError("JSON report and timeline need distinct output paths")
        report, timeline = analyze(args)
        payload = json.dumps(report, indent=2, allow_nan=False) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(payload, encoding="utf-8")
        else:
            print(payload, end="")
        if args.timeline:
            args.timeline.parent.mkdir(parents=True, exist_ok=True)
            columns = list(timeline[0]["timings_ms"])
            counters = list(timeline[0]["native_counters"])
            with args.timeline.open("w", encoding="utf-8", newline="") as stream:
                writer = csv.writer(stream)
                writer.writerow(["capture_frame_index", "elapsed_start_seconds", "event_count"] + [name + "_ms" for name in columns] + [name + "_native" for name in counters])
                for row in timeline:
                    writer.writerow([row["capture_frame_index"], row["elapsed_start_seconds"], row["event_count"]] + [row["timings_ms"][name] for name in columns] + [row["native_counters"][name] for name in counters])
        return 0
    except (OSError, EOFError, UnicodeError, ValueError, csv.Error) as error:
        print(f"Performance analysis failed: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
