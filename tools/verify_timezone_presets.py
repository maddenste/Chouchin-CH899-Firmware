# Copyright (C) 2026 Steve Madden
# SPDX-License-Identifier: GPL-3.0-or-later

"""Verify setup-page timezone presets against the current IANA database.

Requires Python 3, python-dateutil, and tzdata. The setup page remains the
source of truth: this script extracts TIMEZONE_PRESETS directly from it.
"""

from __future__ import annotations

import ast
from datetime import datetime, timedelta, timezone
import importlib.resources as resources
from pathlib import Path
import re

from dateutil.tz import tzstr
import tzdata
from zoneinfo import ZoneInfo


ROOT = Path(__file__).resolve().parent.parent
PAGE = ROOT / "arduino" / "CH899_Clock" / "web" / "index.html"
START = datetime(2026, 9, 10, tzinfo=timezone.utc)
END = datetime(2036, 1, 1, tzinfo=timezone.utc)

ZONES = {
    "baker": ["Etc/GMT+12"],
    "american_samoa": ["Pacific/Pago_Pago"],
    "honolulu": ["Pacific/Honolulu"],
    "anchorage": ["America/Anchorage"],
    "los_angeles": ["America/Los_Angeles"],
    "phoenix_vancouver": ["America/Phoenix", "America/Vancouver"],
    "denver": ["America/Denver"],
    "chicago": ["America/Chicago"],
    "mexico_city": ["America/Mexico_City"],
    "new_york": ["America/New_York"],
    "lima": ["America/Lima"],
    "halifax": ["America/Halifax"],
    "santiago": ["America/Santiago"],
    "newfoundland": ["America/St_Johns"],
    "buenos_aires": ["America/Argentina/Buenos_Aires"],
    "south_georgia": ["Atlantic/South_Georgia"],
    "azores": ["Atlantic/Azores"],
    "utc_reykjavik": ["Etc/UTC", "Atlantic/Reykjavik"],
    "london": ["Europe/London"],
    "paris_berlin": ["Europe/Paris", "Europe/Berlin"],
    "athens": ["Europe/Athens"],
    "johannesburg": ["Africa/Johannesburg"],
    "moscow_nairobi": ["Europe/Moscow", "Africa/Nairobi"],
    "tehran": ["Asia/Tehran"],
    "dubai": ["Asia/Dubai"],
    "karachi": ["Asia/Karachi"],
    "almaty": ["Asia/Almaty"],
    "india_sri_lanka": ["Asia/Kolkata", "Asia/Colombo"],
    "dhaka": ["Asia/Dhaka"],
    "yangon": ["Asia/Yangon"],
    "bangkok_jakarta": ["Asia/Bangkok", "Asia/Jakarta"],
    "singapore_beijing": ["Asia/Singapore", "Asia/Shanghai"],
    "tokyo_seoul": ["Asia/Tokyo", "Asia/Seoul"],
    "darwin": ["Australia/Darwin"],
    "sydney_melbourne": ["Australia/Sydney", "Australia/Melbourne"],
    "solomon": ["Pacific/Guadalcanal"],
    "auckland": ["Pacific/Auckland"],
    "fiji": ["Pacific/Fiji"],
    "samoa_tonga": ["Pacific/Apia", "Pacific/Tongatapu"],
    "kiritimati": ["Pacific/Kiritimati"],
}

ABBREVIATION = r"(?:[A-Za-z]{3,10}|<[A-Za-z0-9+\-]{3,8}>)"
OFFSET = r"[+\-]?(?:[0-9]|1[0-9]|2[0-4])(?::[0-5]?[0-9])?"
TRANSITION = r"M(?:[1-9]|1[0-2])\.[1-5]\.[0-6](?:/(?:[0-9]{1,2}|1[0-6][0-9])(?::[0-5]?[0-9])?)?"
SUPPORTED_RULE = re.compile(
    rf"^{ABBREVIATION}{OFFSET}(?:{ABBREVIATION}(?:{OFFSET})?,{TRANSITION},{TRANSITION})?$"
)


def load_presets() -> list[list[str]]:
    page = PAGE.read_text(encoding="utf-8")
    match = re.search(r"const TIMEZONE_PRESETS = (\[[\s\S]*?\n    \]);", page)
    if not match:
        raise AssertionError("TIMEZONE_PRESETS was not found in the setup page")
    return ast.literal_eval(match.group(1))


def iana_footer(zone: str) -> str:
    data = resources.files("tzdata.zoneinfo").joinpath(*zone.split("/")).read_bytes()
    return data[data.rfind(b"\n", 0, -1) + 1 : -1].decode("ascii")


def dateutil_compatible(rule: str) -> str:
    names = iter(("STD", "DST"))
    return re.sub(r"<[^>]+>", lambda _: next(names), rule)


def verify() -> None:
    presets = load_presets()
    assert len(presets) == len(ZONES) == 40
    assert [preset[0] for preset in presets] == list(ZONES)

    for preset_id, label, standard_rule, automatic_rule in presets:
        assert SUPPORTED_RULE.fullmatch(standard_rule), (preset_id, standard_rule)
        assert SUPPORTED_RULE.fullmatch(automatic_rule), (preset_id, automatic_rule)
        assert len(standard_rule) <= 96 and len(automatic_rule) <= 96
        zones = ZONES[preset_id]

        # An exact TZif footer is IANA's own future POSIX rule. For grouped or
        # deliberately renamed fixed rules, compare every UTC hour instead.
        exact_iana_rule = all(automatic_rule == iana_footer(zone) for zone in zones)
        if not exact_iana_rule:
            automatic = tzstr(dateutil_compatible(automatic_rule), posix_offset=True)
            instant = START
            while instant < END:
                expected = {instant.astimezone(ZoneInfo(zone)).utcoffset() for zone in zones}
                assert len(expected) == 1, (preset_id, instant, expected)
                assert instant.astimezone(automatic).utcoffset() == next(iter(expected)), (
                    preset_id,
                    instant,
                    automatic_rule,
                )
                instant += timedelta(hours=1)

        # Disabled mode must equal the locality's IANA non-DST/base offset.
        standard = tzstr(dateutil_compatible(standard_rule), posix_offset=True)
        expected_standard = None
        for year in range(2027, 2036):
            for month in range(1, 13):
                instant = datetime(year, month, 15, 12, tzinfo=timezone.utc)
                for zone in zones:
                    local = instant.astimezone(ZoneInfo(zone))
                    if not local.dst():
                        if expected_standard is None:
                            expected_standard = local.utcoffset()
                        assert local.utcoffset() == expected_standard, (preset_id, zone, instant)
        assert expected_standard is not None
        assert standard.utcoffset(START) == expected_standard, (preset_id, standard_rule)
        print(f"PASS {preset_id}: {label}")

    print(
        f"VERIFIED {len(presets)}/{len(presets)} presets with IANA tzdata {tzdata.__version__}; "
        "automatic behavior checked from 2026-09-10 through 2035."
    )


if __name__ == "__main__":
    verify()
