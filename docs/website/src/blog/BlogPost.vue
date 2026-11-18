<script setup lang="ts">
import { RouterLink } from 'vue-router'
import type { Page } from 'virtual:content'
import MarkdownBody from '../content/MarkdownBody.vue'
import { formatDate } from './date'

defineProps<{ post: Page }>()
</script>

<template>
  <article class="container-page max-w-3xl py-10">
    <RouterLink to="/blog" class="text-sm text-muted hover:text-fg">&larr; All posts</RouterLink>
    <time :datetime="post.date!" class="mt-6 block text-xs text-muted">
      {{ formatDate(post.date!) }}
    </time>
    <MarkdownBody :page="post" class="mt-2" />
  </article>
</template>
