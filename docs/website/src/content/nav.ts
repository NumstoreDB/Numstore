// How the docs are grouped on the site. This is the one place that orders
// pages; it does not have to follow the folders in docs/.

import { docs, type Page } from 'virtual:content'

export interface Group {
  title?: string
  pages: Page[]
}

export interface Tab {
  label: string
  path: string // the first page of the tab
  groups: Group[]
}

const layout: { label: string; groups: { title?: string; files: string[] }[] }[] = [
  {
    label: 'Get started',
    groups: [
      {
        files: ['README.md', 'concepts/why-numstore.md', 'getting-started.md', 'glossary.md'],
      },
    ],
  },
  {
    label: 'Tutorials',
    groups: [
      {
        files: [
          'tutorials/query-language.md',
          'tutorials/transactions.md',
          'tutorials/type-sizes.md',
          'tutorials/crash-recovery.md',
        ],
      },
    ],
  },
  {
    label: 'Guides',
    groups: [
      {
        title: 'C library',
        files: [
          'guides/build-and-link.md',
          'guides/handle-errors.md',
          'guides/transactions-in-c.md',
          'guides/read-data-in-c.md',
          'guides/structs-and-arrays-in-c.md',
        ],
      },
      {
        title: 'Working with data',
        files: [
          'guides/inspect-variables.md',
          'guides/reuse-queries.md',
          'guides/move-data-in-python.md',
          'guides/back-up-a-database.md',
        ],
      },
    ],
  },
  {
    label: 'Concepts',
    groups: [
      {
        files: [
          'concepts/types.md',
          'concepts/transactions-and-concurrency.md',
          'concepts/durability.md',
          'concepts/errors.md',
        ],
      },
    ],
  },
]

const byFile = new Map(docs.map((p) => [p.file, p]))

function pageFor(file: string): Page {
  const page = byFile.get(file)
  if (!page) throw new Error(`nav.ts lists docs/${file}, but that file does not exist`)
  return page
}

export const tabs: Tab[] = layout.map(({ label, groups }) => {
  const resolved = groups.map((g) => ({ title: g.title, pages: g.files.map(pageFor) }))
  return { label, path: resolved[0].pages[0].path, groups: resolved }
})

// Where "Read the docs" buttons go.
export const startPath = pageFor('getting-started.md').path

const listed = new Set(tabs.flatMap((t) => t.groups.flatMap((g) => g.pages)))
const unlisted = docs.filter((p) => !listed.has(p))
if (import.meta.env.DEV && unlisted.length > 0) {
  console.warn(
    'These docs are not in nav.ts:',
    unlisted.map((p) => p.file),
  )
}

// Unlisted pages still get a route. They show the first tab's sidebar.
export function tabFor(page: Page): Tab {
  return tabs.find((t) => t.groups.some((g) => g.pages.includes(page))) ?? tabs[0]
}

// The pages before and after this one, in tab order.
export function neighbours(page: Page): { prev?: Page; next?: Page } {
  const pages = tabFor(page).groups.flatMap((g) => g.pages)
  const i = pages.indexOf(page)
  if (i < 0) return {}
  return { prev: pages[i - 1], next: pages[i + 1] }
}
