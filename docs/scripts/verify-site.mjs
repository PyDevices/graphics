import { existsSync, readFileSync, readdirSync, statSync } from "node:fs";
import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const docsDir = resolve(fileURLToPath(new URL("..", import.meta.url)));
const repoDir = resolve(docsDir, "..");
const generatedDir = join(docsDir, "generated");
const distDir = join(docsDir, "dist");
const manifest = JSON.parse(
  readFileSync(join(generatedDir, "manifest.json"), "utf8"),
);
const expectedBaseUrl = "/graphics/api/";
const expectedSiteUrl = "https://pydevices.github.io/graphics/api/";

function walk(directory) {
  return readdirSync(directory).flatMap((name) => {
    const path = join(directory, name);
    return statSync(path).isDirectory() ? walk(path) : [path];
  });
}

const failures = [];
if (manifest.python_symbol_count < 70) {
  failures.push(
    `expected at least 70 Python API entries, got ${manifest.python_symbol_count}`,
  );
}
if (manifest.native_c_symbol_count < 20) {
  failures.push(
    `expected at least 20 native C functions, got ${manifest.native_c_symbol_count}`,
  );
}
if (manifest.source_file_count < 5) {
  failures.push(
    `expected at least 5 represented source files, got ${manifest.source_file_count}`,
  );
}
if (manifest.repository_native_source_file_count < 20) {
  failures.push("repository native source inventory is unexpectedly small");
}
if (manifest.pages.length !== 7 || manifest.pages.some((page) => page.symbol_count === 0)) {
  failures.push("expected seven non-empty generated reference pages");
}

for (const required of [
  "index.html",
  "search-index.json",
  "llms.txt",
  "llms-full.txt",
  "sitemap.xml",
]) {
  if (!existsSync(join(distDir, required))) failures.push(`missing dist/${required}`);
}

const expectedSourcePrefix =
  `https://github.com/PyDevices/graphics/blob/${manifest.commit}/`;
if (
  manifest.symbols.some(
    (symbol) =>
      !symbol.source_url.startsWith(expectedSourcePrefix) ||
      !symbol.source_url.endsWith(`#L${symbol.line}`) ||
      !existsSync(join(repoDir, symbol.path)),
  )
) {
  failures.push("one or more entries lack an immutable, existing source declaration");
}

if (existsSync(join(distDir, "search-index.json"))) {
  const searchIndex = JSON.parse(
    readFileSync(join(distDir, "search-index.json"), "utf8"),
  );
  if (
    searchIndex.some(
      (entry) =>
        typeof entry.url !== "string" || !entry.url.startsWith(expectedBaseUrl),
    )
  ) {
    failures.push(`search results are not scoped to ${expectedBaseUrl}`);
  }
  const searchable = searchIndex
    .map((entry) => `${entry.title ?? ""} ${entry.content ?? ""}`)
    .join("\n");
  for (const expected of [
    "graphics.FrameBuffer",
    "graphics.Area.contains",
    "graphics.Draw.clip",
    "graphics.Font.text_width",
    "gfx_shapes_fill_rect",
  ]) {
    if (!searchable.includes(expected)) {
      failures.push(`search index is missing ${expected}`);
    }
  }
}

if (existsSync(distDir)) {
  const htmlFiles = walk(distDir).filter((path) => path.endsWith(".html"));
  const html = htmlFiles.map((path) => readFileSync(path, "utf8")).join("\n");
  if (!html.includes(manifest.commit)) {
    failures.push("site does not expose the immutable source commit");
  }
  if (!html.includes("graphics.FrameBuffer") || !html.includes("gfx_shapes_fill_rect")) {
    failures.push("site is missing representative Python or native C entries");
  }
  const canonicalUrls = [
    ...html.matchAll(/<link rel="canonical" href="([^"]+)"/g),
  ].map((match) => match[1]);
  if (
    canonicalUrls.length !== htmlFiles.length ||
    canonicalUrls.some((url) => !url.startsWith(expectedSiteUrl))
  ) {
    failures.push("one or more pages lack a project-owned canonical URL");
  }
}

const homepage = readFileSync(join(repoDir, "web", "index.html"), "utf8");
if (!homepage.includes('href="api/"')) {
  failures.push("project homepage does not link to the API reference");
}
const readme = readFileSync(join(repoDir, "README.md"), "utf8");
if (!readme.includes("https://pydevices.github.io/graphics/api/")) {
  failures.push("README does not link to the published API reference");
}
const workflow = readFileSync(
  join(repoDir, ".github", "workflows", "deploy-pages.yml"),
  "utf8",
);
for (const required of [
  "npm ci --prefix docs",
  "npm run check --prefix docs",
  "cp -r docs/dist/* _site/api/",
  "fetch-depth: 0",
  "fetch-tags: true",
]) {
  if (!workflow.includes(required)) {
    failures.push(`Pages workflow is missing: ${required}`);
  }
}

if (failures.length > 0) {
  console.error(failures.map((failure) => `- ${failure}`).join("\n"));
  process.exitCode = 1;
} else {
  console.log(
    `Verified ${manifest.python_symbol_count} Python entries and ` +
      `${manifest.native_c_symbol_count} native C functions from ` +
      `${manifest.source_file_count} source files.`,
  );
}
