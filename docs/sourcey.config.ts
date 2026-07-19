import { defineConfig, markdown } from "sourcey";

export default defineConfig({
  name: "graphics",
  repo: "https://github.com/PyDevices/graphics",
  siteUrl: "https://pydevices.github.io",
  baseUrl: "/graphics/api/",
  favicon: "./favicon.svg",
  editBranch: "main",
  editBasePath: "docs",
  theme: {
    preset: "api-first",
    colors: {
      primary: "#d97706",
      light: "#f59e0b",
      dark: "#92400e",
    },
  },
  navbar: {
    links: [
      {
        type: "github",
        href: "https://github.com/PyDevices/graphics",
      },
    ],
    primary: {
      type: "button",
      label: "Project home",
      href: "https://pydevices.github.io/graphics/",
    },
  },
  navigation: {
    tabs: [
      {
        tab: "API Reference",
        slug: "",
        source: markdown({
          groups: [
            {
              group: "Overview",
              pages: ["generated/introduction"],
            },
            {
              group: "Python API",
              pages: [
                "generated/area-clipping",
                "generated/framebuffer",
                "generated/drawing",
                "generated/fonts-images",
                "generated/module-formats",
              ],
            },
            {
              group: "Native C API",
              pages: [
                "generated/native-drawing",
                "generated/native-assets-runtime",
              ],
            },
          ],
        }),
      },
    ],
  },
});
