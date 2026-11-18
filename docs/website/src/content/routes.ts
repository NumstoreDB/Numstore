// Maps a Markdown file under docs/ to its URL on the site. The build plugin
// (link rewriting) and the router both use it, so links and routes agree.

// docs/blog/YYYY/MM/DD/<slug>.md
export const POST_FILE = /^blog\/(\d{4})\/(\d{2})\/(\d{2})\/([^/]+)\.md$/

export function routeFor(file: string): string {
  const post = POST_FILE.exec(file)
  if (post) return `/blog/${post[4]}`

  if (file === 'README.md') return '/docs'

  // guides/read-data-in-c.md -> /docs/guides/read-data-in-c
  const slug = file.replace(/\.md$/, '').toLowerCase().replace(/_/g, '-')
  return `/docs/${slug}`
}
