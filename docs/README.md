# API reference site

This directory contains the reproducible Sourcey reference for the native
`graphics` module. The generator reads the CPython binding tables and public C
headers at the checked-out commit, then emits source-linked Markdown for the
Python and native integration surfaces.

```bash
cd docs
npm ci
npm run check
```

`npm run check` regenerates the reference, builds the static site into `dist/`,
and verifies API coverage, immutable source links, project-scoped search URLs,
canonical URLs, and integration with the existing Pages deployment. Generated
HTML remains untracked build output.

The existing Pages workflow preserves the project homepage and publishes the
Sourcey build below `/graphics/api/` after a merge to `main`.
