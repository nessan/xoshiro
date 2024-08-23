# README

This directory holds content and code for the library's [documentation site][], built using the static website generator Quarto. The site's content is in [Quarto Markdown][].

| File/Directory   | Purpose                                                                                  |
| ---------------- | ---------------------------------------------------------------------------------------- |
| `_quarto.yml`    | The site's configuration file setting the theme, navigation menus, etc.                  |
| `_extensions/`   | Quarto extensions are added to this top-level directory and enabled in `_quarto.yml`     |
| `_variables.yml` | Variable definitions for Quarto's `{{<var ...>}}` shortcode & our `simple-vars` version. |
| `assets/`        | Assets such as `CSS` snippets used to customise the theme chosen for the site.           |
| `pages/`         | Contains the site’s content written in Quarto markdown (files with a `.qmd` extension).  |
| `index.qmd`      | This is the site's landing page, a link to `pages/index.qmd` directory.                  |
| `_site/`         | Where Quarto puts the built site. It does not need to be checked into `git`.             |
| `.quarto/`       | A cache used by Quarto that does not need to be checked into `git`.                      |
| `.luarc.json`    | Configuration used by `VSCode` that does not need to be checked into `git`.              |

## Note

We extensively use Quarto's ability to insert content from the project-level `_variables.yml` file.

In that file, we define variables that point to the library's documentation for each class, method, and function. These nicely formatted links are used throughout the site.

In Quarto, the canonical way to refer to a variable is by using the `var` shortcode like `{{< var name >}}`. This is a little wordy when we have a lot of variables. The [`simple-vars`][] Quarto extension allows us to replace those references with the simple markup `{name}`.

<!-- Reference links -->

[documentation site]: https://nessan.github.io/xoshiro
[Quarto]: https://quarto.org
[Quarto Markdown]: https://quarto.org/pages/authoring/markdown-basics.html
[`simple-vars`]: https://github.com/nessan/simple-vars
