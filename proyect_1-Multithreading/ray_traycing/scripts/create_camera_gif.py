"""@file create_camera_gif.py
@brief Assembles numbered PPM camera-orbit frames into a looping GIF.
"""

import argparse
from pathlib import Path

from PIL import Image


def main():
    """Read the frame directory, output path, and per-frame duration from CLI."""
    parser = argparse.ArgumentParser()
    parser.add_argument("frames_directory", type=Path, help="directory with frame_*.ppm files")
    parser.add_argument("output", type=Path, help="destination GIF path")
    parser.add_argument("frame_duration_ms", type=int, help="duration of each frame in milliseconds")
    args = parser.parse_args()

    frame_paths = sorted(args.frames_directory.glob("frame_*.ppm"))
    if not frame_paths:
        parser.error(f"no PPM frames found in {args.frames_directory}")

    frames = [Image.open(path).convert("RGB") for path in frame_paths]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    frames[0].save(
        args.output,
        format="GIF",
        save_all=True,
        append_images=frames[1:],
        duration=args.frame_duration_ms,
        loop=0,
        disposal=2,
        optimize=False,
    )
    for frame in frames:
        frame.close()


if __name__ == "__main__":
    main()