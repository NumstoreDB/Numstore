<script setup lang="ts">
import { RouterLink } from 'vue-router'
import type { Page } from 'virtual:content'
import MarkdownBody from '../content/MarkdownBody.vue'
import { neighbours, tabFor } from '../content/nav'
import SideNav from './SideNav.vue'

const props = defineProps<{ page: Page }>()
const tab = tabFor(props.page)
const { prev, next } = neighbours(props.page)
</script>

<template>
  <div class="container-page grid gap-8 py-10 md:grid-cols-[14rem_minmax(0,1fr)] md:gap-12">
    <aside class="md:sticky md:top-24 md:self-start">
      <!-- Small screens: fold the list so the page text comes first. -->
      <details class="border border-border md:hidden">
        <summary class="cursor-pointer px-4 py-3 text-sm font-medium">
          {{ tab.label }}: pages in this section
        </summary>
        <div class="border-t border-border p-4">
          <SideNav :tab="tab" :current="page" />
        </div>
      </details>
      <SideNav :tab="tab" :current="page" class="hidden md:block" />
    </aside>

    <article class="min-w-0 max-w-3xl">
      <MarkdownBody :page="page" />

      <nav
        v-if="prev || next"
        class="mt-16 grid gap-4 border-t border-border pt-6 text-sm sm:grid-cols-2"
        aria-label="Previous and next page"
      >
        <RouterLink v-if="prev" :to="prev.path" class="border border-border p-4 hover:bg-elevated">
          <span class="block text-xs text-muted">Previous</span>
          {{ prev.title }}
        </RouterLink>
        <span v-else />
        <RouterLink
          v-if="next"
          :to="next.path"
          class="border border-border p-4 text-right hover:bg-elevated"
        >
          <span class="block text-xs text-muted">Next</span>
          {{ next.title }}
        </RouterLink>
      </nav>
    </article>
  </div>
</template>
