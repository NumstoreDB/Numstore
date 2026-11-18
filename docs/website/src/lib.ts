import hljs from 'highlight.js/lib/core'
import bash from 'highlight.js/lib/languages/bash'
import c from 'highlight.js/lib/languages/c'
import python from 'highlight.js/lib/languages/python'
import rust from 'highlight.js/lib/languages/rust'
import 'highlight.js/styles/github-dark.css'

type Result<T> =
  | { success: true; data: T }
  | { success: false; error: string };

////////////////////// Code Highlighting

hljs.registerLanguage('bash', bash)
hljs.registerLanguage('c', c)
hljs.registerLanguage('python', python)
hljs.registerLanguage('rust', rust)

export function highlightCode(code: string, language: string): string {
  try {
    return hljs.highlight(code, { language, ignoreIllegals: true }).value
  } catch {
    return hljs.highlight(code, { language: 'plaintext', ignoreIllegals: true }).value
  }
}

////////////////////// Github Stars Cache

export function getStarsFromCache(TTL: number, cacheKey: string): Result<number | undefined> {
  try {
    // Get raw data from session storage
    const raw = sessionStorage.getItem(cacheKey)

    // If it exists - parse it as json
    if (raw) {
      const parsed = JSON.parse(raw) as { stars: number; t: number }
      if (Date.now() - parsed.t < TTL) {
        return { success: true, data: parsed.stars }
      }
    }

    return { success: true, data: undefined }
  } catch (e) {
    return { success: false, error: `Error: ${e}` }
  }
}

export async function fetchGithubStars(owner: string, repo: string, cacheKey: string): Promise<Result<number>> {
  try {
    // Do fetch
    const res = await fetch(
      `https://api.github.com/repos/${owner}/${repo}`,
      { 
        headers: { 
          Accept: 'application/vnd.github+json' 
        } 
      },
    )

    // Failed to fetch status
    if (!res.ok) {
      return { success: false, error: `Github response: ${res.status}` }
    }

    // Get the data
    const ghres = await res.json() as { stargazers_count?: number }

    // Parse stargazers_count
    if(typeof ghres.stargazers_count == 'number') {

      const cacheItem = { data: ghres.stargazers_count, t: Date.now() }
      const cacheItemStr = JSON.stringify(cacheItem)
      sessionStorage.setItem(cacheKey, cacheItemStr)
      return { success: true, data: cacheItem.data }

    } else {
      return { success: false, error: "Failed to parse github response" }
    }
  } catch(e)  {
      return { success: false, error: `Error: ${e}`}
  }
}

