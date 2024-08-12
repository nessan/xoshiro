# README

This directory holds content and code for the library's [documentation site][], which was built using the static website generator Quarto. The site's content is in [Quarto Markdown][] based on the extendible [Pandoc][] version.

| File/Directory   | Purpose                                                                                 |
| ---------------- | --------------------------------------------------------------------------------------- |
| `_quarto.yml`    | The site's configuration file setting the theme, navigation menus, etc.                 |
| `_extensions/`   | If we use any extensions, they get added to this top-level directory.                   |
| `_variables.yml` | Variable definitions for Quarto's `{{<var ...>}}` short-code.                           |
| `assets/`        | Assets such as `CSS` snippets used to customize the theme chosen for the site.          |
| `pages/`         | Contains the site’s content written in Quarto markdown (files with a `.qmd` extension). |
| `index.qmd`      | This is the site's landing page — a link to a file in the `pages` directory.            |
| `_site/`         | Where Quarto puts the built site. It does not need to be checked into `git`.            |
| `.quarto/`       | A cache used by Quarto that does not need to be checked into `git`.                     |
| `.luarc.json`    | Configuration used by `VSCode` that does not need to be checked into `git`.             |

<!-- Reference links -->

[documentation site]: https://nessan.github.io/xoshiro
[Quarto]: https://quarto.org
[Quarto Markdown]: https://quarto.org/pages/authoring/markdown-basics.html
[Pandoc]: https://pandoc.org
