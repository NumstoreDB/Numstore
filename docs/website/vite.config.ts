import { fileURLToPath } from 'node:url'
import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { content } from './plugins/content'

// The site lives in docs/website and renders the Markdown around it.
const docsRoot = fileURLToPath(new URL('..', import.meta.url))

export default defineConfig({
  plugins: [
    content({
      root: docsRoot,
      exclude: ['website'],
      sourceUrl: 'https://github.com/NumstoreDB/Numstore/blob/develop',
    }),
    vue({ include: [/\.vue$/, /\.md$/] }),
  ],
  server: {
    fs: { allow: [docsRoot] },
  },
})
