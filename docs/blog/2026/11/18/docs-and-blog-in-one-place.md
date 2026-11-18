Docs and blog in one place
==========================

The Numstore docs and this blog now live in the same repo as the code. Every
page on this site is a plain Markdown file in `docs/`, so you can read it on
GitHub, in your editor, or here.

Posts go in `docs/blog/YYYY/MM/DD/<slug>.md`. The date comes from the folders
and the title comes from the first heading. All posts show up in one list,
newest first.

To write a new post, add a Markdown file in a dated folder and run:

```sh
make website
```
