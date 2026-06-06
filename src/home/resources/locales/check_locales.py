#!/usr/bin/env python3
"""Assert every locale .ts has the same messages as en.ts with no translation gaps."""

from __future__ import annotations

import argparse
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

MessageKey = tuple[str, str]


def load_messages(ts_path: Path) -> dict[MessageKey, ET.Element]:
    root = ET.parse(ts_path).getroot()
    messages: dict[MessageKey, ET.Element] = {}
    for ctx in root.findall("context"):
        name = ctx.find("name")
        if name is None or not name.text:
            continue
        for msg in ctx.findall("message"):
            source = msg.find("source")
            translation = msg.find("translation")
            if source is None or not source.text or translation is None:
                continue
            key = (name.text, source.text)
            messages[key] = translation
    return messages


def allowed_extra(key: MessageKey, locales: set[str]) -> bool:
    context, source = key
    return context == "Language" and source in locales


def check_locale(locale: str, messages: dict[MessageKey, ET.Element], reference: set[MessageKey], locales: set[str]) -> list[str]:
    errors: list[str] = []
    keys = set(messages)

    for key in sorted(reference - keys):
        errors.append(f"{locale}: missing {key[0]}/{key[1]}")

    for key in sorted(keys - reference):
        if not allowed_extra(key, locales):
            errors.append(f"{locale}: unexpected {key[0]}/{key[1]}")

    for key, translation in sorted(messages.items()):
        ttype = translation.get("type")
        text = (translation.text or "").strip()

        if ttype == "vanished":
            errors.append(f"{locale}: vanished {key[0]}/{key[1]}")
            continue
        if ttype == "unfinished":
            errors.append(f"{locale}: unfinished {key[0]}/{key[1]}")
            continue
        if locale != "en" and not text:
            errors.append(f"{locale}: empty {key[0]}/{key[1]}")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ts-dir", type=Path, required=True)
    parser.add_argument("--reference", default="en")
    parser.add_argument("--locale", action="append", required=True)
    args = parser.parse_args()

    locales = set(args.locale)
    ref_path = args.ts_dir / f"{args.reference}.ts"
    if not ref_path.exists():
        print(f"missing reference {ref_path}", file=sys.stderr)
        return 1

    reference = set(load_messages(ref_path))
    errors: list[str] = []

    for locale in sorted(locales):
        ts_path = args.ts_dir / f"{locale}.ts"
        if not ts_path.exists():
            errors.append(f"{locale}: missing {ts_path}")
            continue
        errors.extend(check_locale(locale, load_messages(ts_path), reference, locales))

    if errors:
        for item in errors:
            print(item, file=sys.stderr)
        return 1

    print(f"OK: {len(reference)} messages complete in {', '.join(sorted(locales))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
