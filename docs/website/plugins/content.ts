// Turns the Markdown in docs/ into Vue components at build time.
//
// - `import x from '<docs>/foo.md'` gives a Vue component that renders the page.
// - `virtual:content` lists every doc and blog post with its route and title.
//
// docs/ stays plain Markdown. Nothing here writes files into it.

import fs from 'node:fs'
import path from 'node:path'
import hljs from 'highlight.js'
import MarkdownIt from 'markdown-it'
import type { Plugin, ViteDevServer } from 'vite'
import { POST_FILE, routeFor } from '../src/content/routes'

type Token = ReturnType<MarkdownIt['parse']>[number]

export interface ContentOptions {
  // Absolute path of the docs/ folder.
  root: string
  // Folders under root that are not content (for example the website itself).
  exclude: string[]
  // Links to files that are not pages go here, e.g. a GitHub blob URL.
  sourceUrl: string
}

interface Page {
  file: string // relative to root, '/' separated
  path: string
  title: string
  date: string | null
  excerpt: string
  html: string
}

const VIRTUAL_ID = 'virtual:content'
const RESOLVED_VIRTUAL_ID = '\0' + VIRTUAL_ID

export function content(opts: ContentOptions): Plugin {
  const md = new MarkdownIt({ html: false, highlight })

  ////////////////////// Files

  function listFiles(dir = ''): string[] {
    const out: string[] = []
    for (const entry of fs.readdirSync(path.join(opts.root, dir), { withFileTypes: true })) {
      const rel = path.posix.join(dir, entry.name)
      if (entry.name.startsWith('.') || opts.exclude.includes(rel)) continue
      if (entry.isDirectory()) out.push(...listFiles(rel))
      else if (entry.name.endsWith('.md')) out.push(rel)
    }
    return out.sort()
  }

  // The file for a module id, or null if the id is not a page under root.
  function fileOf(id: string): string | null {
    if (!id.endsWith('.md')) return null
    const rel = path.relative(opts.root, id).split(path.sep).join('/')
    if (rel.startsWith('../') || opts.exclude.some((ex) => rel.startsWith(ex + '/'))) return null
    return rel
  }

  ////////////////////// Rendering

  function render(file: string): Page {
    if (file.startsWith('blog/') && !POST_FILE.test(file)) {
      throw new Error(`docs/${file}: blog posts go in blog/YYYY/MM/DD/<slug>.md`)
    }

    const src = fs.readFileSync(path.join(opts.root, file), 'utf8')
    const env = {}
    const tokens = md.parse(src, env)

    addHeadingIds(tokens)
    rewriteLinks(tokens, file)

    const title = textAfter(tokens, (t) => t.type === 'heading_open' && t.tag === 'h1')
    if (!title) throw new Error(`docs/${file}: the page needs a level 1 heading`)

    const post = POST_FILE.exec(file)
    return {
      file,
      path: routeFor(file),
      title,
      date: post ? `${post[1]}-${post[2]}-${post[3]}` : null,
      excerpt: textAfter(tokens, (t) => t.type === 'paragraph_open') ?? '',
      html: md.renderer.render(tokens, md.options, env),
    }
  }

  // GitHub style ids, so the existing #anchor links in docs/ keep working.
  function addHeadingIds(tokens: Token[]) {
    const seen = new Map<string, number>()
    tokens.forEach((t, i) => {
      if (t.type !== 'heading_open') return
      const base = slugify(inlineText(tokens[i + 1]))
      const n = seen.get(base) ?? 0
      seen.set(base, n + 1)
      t.attrSet('id', n === 0 ? base : `${base}-${n}`)
    })
  }

  // foo.md -> its site route. Other relative files -> the source repository.
  function rewriteLinks(tokens: Token[], from: string) {
    for (const t of tokens) {
      for (const child of t.children ?? []) {
        if (child.type !== 'link_open') continue
        const href = child.attrGet('href') ?? ''

        if (/^https?:/.test(href)) {
          child.attrSet('target', '_blank')
          child.attrSet('rel', 'noopener')
          continue
        }
        if (/^[a-z][a-z0-9+.-]*:/i.test(href) || href.startsWith('#')) continue

        const [, target, suffix] = /^([^#?]*)(.*)$/.exec(href)!
        const file = path.posix.normalize(path.posix.join(path.posix.dirname(from), target))

        if (file.endsWith('.md') && !file.startsWith('../')) {
          if (!fs.existsSync(path.join(opts.root, file))) {
            throw new Error(`docs/${from}: broken link ${href}`)
          }
          child.attrSet('href', routeFor(file) + suffix)
        } else {
          child.attrSet('href', `${opts.sourceUrl}/${path.posix.join('docs', file)}${suffix}`)
        }
      }
    }
  }

  // v-pre: the HTML is final, so Vue must not read {{ }} in code samples.
  function toSfc(page: Page): string {
    return `<template><div class="doc" v-pre>${page.html}</div></template>\n`
  }

  function toVirtualModule(): string {
    const pages = listFiles().map(render)
    const entry = (p: Page) =>
      `{ file: ${json(p.file)}, path: ${json(p.path)}, title: ${json(p.title)}, ` +
      `date: ${json(p.date)}, excerpt: ${json(p.excerpt)}, ` +
      `load: () => import(${json(path.join(opts.root, p.file))}) }`

    const docs = pages.filter((p) => !p.date)
    const posts = pages.filter((p) => p.date).sort((a, b) => b.date!.localeCompare(a.date!))

    const routes = new Map<string, string>()
    for (const p of pages) {
      const other = routes.get(p.path)
      if (other) throw new Error(`docs/${p.file} and docs/${other} both map to ${p.path}`)
      routes.set(p.path, p.file)
    }

    return (
      `export const docs = [\n${docs.map(entry).join(',\n')}\n]\n` +
      `export const posts = [\n${posts.map(entry).join(',\n')}\n]\n`
    )
  }

  ////////////////////// Dev server: new, removed or renamed pages change the page list

  function watch(server: ViteDevServer) {
    // A folder, not a '**/*.md' glob: the glob did not report new files.
    server.watcher.add(opts.root)
    const onChange = (file: string) => {
      if (!fileOf(file)) return
      const mod = server.moduleGraph.getModuleById(RESOLVED_VIRTUAL_ID)
      if (mod) server.reloadModule(mod)
    }
    server.watcher.on('add', onChange)
    server.watcher.on('unlink', onChange)
    server.watcher.on('change', onChange)
  }

  return {
    name: 'numstore-content',
    enforce: 'pre',
    configureServer: watch,
    resolveId: (id) => (id === VIRTUAL_ID ? RESOLVED_VIRTUAL_ID : null),
    load: (id) => (id === RESOLVED_VIRTUAL_ID ? toVirtualModule() : null),
    transform(_code, id) {
      const file = fileOf(id)
      return file ? toSfc(render(file)) : null
    },
  }
}

////////////////////// Helpers

function highlight(code: string, lang: string): string {
  if (!lang || !hljs.getLanguage(lang)) return '' // markdown-it escapes it
  return hljs.highlight(code, { language: lang, ignoreIllegals: true }).value
}

function inlineText(t: Token | undefined): string {
  return (t?.children ?? [])
    .map((c) => {
      if (c.type === 'text' || c.type === 'code_inline') return c.content
      if (c.type === 'softbreak' || c.type === 'hardbreak') return ' '
      return ''
    })
    .join('')
}

// The text of the inline token right after the first token that matches.
function textAfter(tokens: Token[], match: (t: Token) => boolean): string | null {
  const i = tokens.findIndex(match)
  return i < 0
    ? null
    : inlineText(tokens[i + 1])
        .replace(/\s+/g, ' ')
        .trim()
}

function slugify(text: string): string {
  return text
    .trim()
    .toLowerCase()
    .replace(/[^\p{L}\p{N}\s_-]/gu, '')
    .replace(/\s/g, '-')
}

function json(v: unknown): string {
  return JSON.stringify(v)
}
