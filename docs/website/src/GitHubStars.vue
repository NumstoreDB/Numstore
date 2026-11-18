<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { site_data, icons } from './data'
import { fetchGithubStars, getStarsFromCache } from './lib'

// The number of github stars on the repo
const stars = ref<number | null>(null)

// Whether fetch failed
const failed = ref(false)

// Data
const repoUrl = `https://github.com/${site_data.github.owner}/${site_data.github.repo}`
const cacheKey = `numstore:gh-stars:${site_data.github.owner}/${site_data.github.repo}`
const TTL = 60 * 60 * 1000

onMounted(async () => {
  // First, fetch from cache
  const cachedStars = getStarsFromCache(TTL, cacheKey)

  if (cachedStars.success) {
    if (cachedStars.data != undefined) {
      stars.value = cachedStars.data
    }
  }

  // If that failed, fetch from the network
  const response = await fetchGithubStars(
    site_data.github.owner,
    site_data.github.repo,
    cacheKey
  )

  if (response.success) {
    stars.value = response.data
  }
})
</script>

<template>
  <a  
    :href="repoUrl"
    target="_blank"
    rel="noopener"
    class="inline-flex h-9 items-center gap-1.5 border border-border px-2.5 text-xs font-medium text-muted transition-colors hover:text-fg"
    :aria-label="stars !== null ? `${stars} stars on GitHub` : 'GitHub'"
  >
    <svg viewBox="0 0 24 24" class="h-4 w-4" fill="currentColor" aria-hidden="true">
      <path :d="icons.github" />
    </svg>
    <span v-if="stars !== null" class="tabular-nums">{{ stars }}</span>
    <span v-else-if="failed" class="sr-only">GitHub</span>
    <span v-else class="text-muted/60 tabular-nums">···</span>
  </a>
</template>
