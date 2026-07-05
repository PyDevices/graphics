# Recovery notes (cloud agent bc-2f82623a)

The native **graphics** cmod was developed on a Cursor cloud VM at
`/workspace/graphics` on branch `cursor/native-framebuf-plus-67c8`, but
**never pushed** — `gh repo create PyDevices/graphics` failed for `cursor[bot]`
(organization repo creation requires a maintainer PAT).

## What landed on GitHub

- [PyDevices/cmods#2](https://github.com/PyDevices/cmods/pull/2) — manifest wiring
  (`package("graphics", base_path="graphics/py")`, `clone_profile.sh` graphics profile)
- This empty repo placeholder

## What was lost (VM-only)

- Full native C module tree (`graphics/` cmod, tests, `test_area.py`, etc.)
- Branch `cursor/native-framebuf-plus-67c8`

## How to recover

1. Re-run cloud agent [bc-2f82623a](https://cursor.com/agents/bc-2f82623a-4ba6-465f-add1-23d0084667c8)
   with a **multi-repo environment** including `PyDevices/graphics`, or
2. Re-implement from cmods PR #2 description and pydisplay `framebuf` shim needs, or
3. Push a local clone if you have a backup.

After content exists, push `cursor/native-framebuf-plus-67c8` (or equivalent) and
open a PR here. Merge [cmods#2](https://github.com/PyDevices/cmods/pull/2) after
the graphics repo has the expected `graphics/py` layout.
