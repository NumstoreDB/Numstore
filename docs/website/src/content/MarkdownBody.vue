<script setup lang="ts">
import { defineAsyncComponent } from 'vue'
import { useRouter } from 'vue-router'
import type { Page } from 'virtual:content'
import 'highlight.js/styles/github-dark.css'

const props = defineProps<{ page: Page }>()
const Body = defineAsyncComponent(props.page.load)
const router = useRouter()

// The rendered Markdown has plain <a> tags. Send links to site pages and
// anchors through the router, so they do not reload the whole app.
function onClick(e: MouseEvent) {
  if (e.defaultPrevented || e.button !== 0 || e.metaKey || e.ctrlKey || e.shiftKey || e.altKey) {
    return
  }
  const link = (e.target as Element).closest('a')
  const href = link?.getAttribute('href')
  if (!link || !href || link.target) return

  if (href.startsWith('#')) {
    e.preventDefault()
    router.push({ hash: href })
  } else if (href.startsWith('/')) {
    e.preventDefault()
    router.push(href)
  }
}
</script>

<template>
  <div @click="onClick">
    <Body />
  </div>
</template>
