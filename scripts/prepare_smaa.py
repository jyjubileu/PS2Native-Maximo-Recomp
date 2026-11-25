"""Prepare the pinned reference SMAA 1x tables from local upstream DDS files."""
import argparse
import hashlib
from pathlib import Path

AREA_DDS_SHA256 = "d0a59111b18e67a66341bf94eeeb96caab0dc120e356bbfd934397a5b1830a73"
SEARCH_DDS_SHA256 = "891e444b6dde7dd98f6902c805cd04e3469892aa1667c92f764d94789477ef3a"
DDS_HEADER_BYTES = 128
AREA_WIDTH = 160
AREA_PLANE_HEIGHT = 80


def dds_payload(path, expected):
    data = path.read_bytes()
    if not data.startswith(b"DDS ") or hashlib.sha256(data).hexdigest() != expected:
        raise ValueError(f"Unexpected upstream SMAA file: {path.name}")
    return data[DDS_HEADER_BYTES:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("textures", type=Path, help="Local folder with the pinned AreaTexDX10.dds and SearchTex.dds")
    parser.add_argument("--out", type=Path, default=Path(__file__).resolve().parents[1] / ".local" / "smaa")
    args = parser.parse_args()
    try:
        area = dds_payload(args.textures / "AreaTexDX10.dds", AREA_DDS_SHA256)
        search = dds_payload(args.textures / "SearchTex.dds", SEARCH_DDS_SHA256)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    # SMAA 1x uses only the zero-offset plane. The DDS stores BGR8.
    bgr = area[:AREA_WIDTH * AREA_PLANE_HEIGHT * 3]
    rg = bytearray(AREA_WIDTH * AREA_PLANE_HEIGHT * 2)
    rg[0::2], rg[1::2] = bgr[2::3], bgr[1::3]
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "area.rg8").write_bytes(rg)
    (args.out / "search.r8").write_bytes(search)


if __name__ == "__main__":
    main()
