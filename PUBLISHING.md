# Publishing and releases

How changes in this repo become versioned **`graphics-cmod`** wheels on [TestPyPI](https://test.pypi.org/project/graphics-cmod/), and how to install them.

The CPython package ships a native **`graphics_native`** extension plus the pure-Python **`graphics`** package. CI builds platform wheels with [cibuildwheel](https://cibuildwheel.pypa.io/):

| Platform | Wheel tag |
|----------|-----------|
| Linux x86_64 | `manylinux_*` |
| Windows x64 | `win_amd64` |
| Android arm64 | `android_21_arm64_v8a` (`cp313`, `cp314`) |
| Android x86_64 (emulator) | `android_21_x86_64` (`cp313`, `cp314`) |

## Pipeline overview

```text
graphics (your machine)
  commit → push main
           │
           ▼
  ./scripts/publish_release_tag.sh --push   (or manual git tag vX.Y.Z)
           │
           ▼
graphics: Publish TestPyPI                    (on tag push v*.*.* or workflow_dispatch)
  cibuildwheel → Linux + Windows + Android wheels → twine upload
```

## Version numbers

Format: **`X.Y.Z`** (semver)

| Part | Source |
|------|--------|
| **First release** | `RELEASE_VERSION` in `setup.py` when no `v*.*.*` tags exist yet (**`0.0.1`**) |
| **Later releases** | highest existing tag + 1 patch (`v0.0.1` → `v0.0.2`, …) |

Preview the next version:

```bash
./scripts/next_release_version.sh --verbose
```

TestPyPI rejects re-uploading the same version — each release needs a new tag (handled automatically by `publish_release_tag.sh`).

## One-time setup

### TestPyPI token (organization secret)

`TESTPYPI_API_TOKEN` can live at the **PyDevices organization** level:

1. **PyDevices** org → **Settings** → **Secrets and variables** → **Actions**
2. Organization secret name: `TESTPYPI_API_TOKEN`
3. Grant access to **`graphics`** (and other publish repos as needed)

Workflows reference the secret by name — no YAML changes required. If a repository also defines the same secret locally, the **repo secret overrides** the org secret.

### Optional repository secret

| Secret | Required | Purpose |
|--------|----------|---------|
| `RELEASE_WORKFLOW_TOKEN` | only for future CI that tags from `GITHUB_TOKEN` | PAT with **`actions:write`** on this repo — tag pushes from `GITHUB_TOKEN` do not trigger other workflows |

For manual or local tag pushes, `TESTPYPI_API_TOKEN` alone is enough.

## Release (local clone)

```bash
git push origin main
./scripts/publish_release_tag.sh --push
```

Preview without tagging:

```bash
./scripts/next_release_version.sh --verbose
./scripts/publish_release_tag.sh --dry-run
```

## Manual release (GitHub CLI, no clone)

```bash
gh workflow run publish-testpypi.yml --repo PyDevices/graphics -f version=0.0.1
gh run list --repo PyDevices/graphics
gh run watch --repo PyDevices/graphics
```

## GitHub Actions workflows

| Workflow | Trigger | What it does |
|----------|---------|--------------|
| [publish-testpypi.yml](.github/workflows/publish-testpypi.yml) | Tag push `v*.*.*`; workflow_dispatch | **cibuildwheel**: Linux manylinux + Windows amd64 + Android (`android_21_arm64_v8a`, `android_21_x86_64`) → TestPyPI upload |

## Scripts

| Script | Purpose |
|--------|---------|
| `scripts/next_release_version.sh` | Print next semver (`RELEASE_VERSION` or highest tag + 1 patch) |
| `scripts/publish_release_tag.sh` | Create annotated tag `vX.Y.Z` and optionally push (triggers publish) |

## Local wheel builds (cibuildwheel)

Reproduce CI wheels locally:

```bash
pipx install cibuildwheel   # one-time
echo "0.0.0.dev" > VERSION
pipx run cibuildwheel --platform linux    # or --platform windows
ls wheelhouse/
```

**Linux requires Docker** for manylinux wheels. **Windows** needs MSVC Build Tools on a native Windows shell.

**Android (PEP 738):** needs the Android SDK on Linux or macOS:

```bash
echo "0.0.0.dev" > VERSION
pipx run cibuildwheel --platform android
ls wheelhouse/*android*.whl
```

Dev-only smoke test without cibuildwheel:

```bash
echo "0.0.0.dev" > VERSION
python3 -m venv .venv
.venv/bin/pip install -e .
.venv/bin/python test_graphics.py
.venv/bin/python test_area.py
```

## Install from TestPyPI

Wheels are built for **CPython 3.10–3.14** (one wheel per minor × platform). Pip picks the tag matching your interpreter.

```bash
pip install -i https://test.pypi.org/simple/ graphics-cmod
```

Import as usual:

```python
import graphics
from graphics import FrameBuffer, RGB565, framebuf_backend
```

## Troubleshooting

| Symptom | Likely cause |
|---------|----------------|
| Publish fails: 403 on TestPyPI | Bad or missing `TESTPYPI_API_TOKEN`; confirm org secret grants access to this repo |
| Publish fails: 400 duplicate version | Tag already uploaded; publish the next patch |
| Publish fails on Windows only | Check MSVC build logs in the `windows-latest` matrix job |
| pip: no matching distribution | No wheel for your CPython minor / platform — check files on TestPyPI |
| Local cibuildwheel: `FileNotFoundError: 'docker'` | Linux manylinux builds need Docker locally; CI has it |
| `publish_release_tag.sh`: uncommitted changes | Commit or stash before tagging |
