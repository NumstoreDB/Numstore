/// <reference types="vite/client" />

declare module '*.vue' {
  import type { DefineComponent } from 'vue'
  const component: DefineComponent<{}, {}, any>
  export default component
}

// Made by plugins/content.ts from the Markdown in docs/.
declare module 'virtual:content' {
  import type { Component } from 'vue'

  export interface Page {
    file: string // relative to docs/, e.g. 'howto/read_in_chunks.md'
    path: string // site route
    title: string // the first level 1 heading
    date: string | null // YYYY-MM-DD for blog posts, null for docs
    excerpt: string // the first paragraph as plain text
    load: () => Promise<{ default: Component }>
  }

  export const docs: Page[]
  export const posts: Page[] // newest first
}

interface ImportMetaEnv {
  readonly VITE_YOUTUBE_API_KEY?: string
}

interface ImportMeta {
  readonly env: ImportMetaEnv
}
