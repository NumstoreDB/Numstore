<script setup lang="ts">
import { RouterLink } from 'vue-router'
import type { Page } from 'virtual:content'
import type { Tab } from '../content/nav'

defineProps<{ tab: Tab; current: Page }>()
</script>

<template>
  <nav class="space-y-6 text-sm" aria-label="Pages in this section">
    <div v-for="(group, i) in tab.groups" :key="i">
      <h2
        v-if="group.title"
        class="mb-2 font-sans text-xs font-semibold uppercase tracking-widest text-muted"
      >
        {{ group.title }}
      </h2>
      <ul class="space-y-1">
        <li v-for="p in group.pages" :key="p.path">
          <RouterLink
            :to="p.path"
            class="block border-l px-3 py-1 transition-colors"
            :class="
              p === current
                ? 'border-fg text-fg'
                : 'border-border text-muted hover:border-muted hover:text-fg'
            "
          >
            {{ p.title }}
          </RouterLink>
        </li>
      </ul>
    </div>
  </nav>
</template>
