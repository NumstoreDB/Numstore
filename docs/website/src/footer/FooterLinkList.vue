<script setup lang="ts">
import { RouterLink } from 'vue-router'

export interface FooterLink {
  label: string
  to?: string // internal route
  href?: string // external or mailto
}

defineProps<{ title: string; items: FooterLink[] }>()
</script>

<template>
  <div>
    <h4 class="text-xs font-semibold uppercase tracking-widest text-muted">{{ title }}</h4>
    <ul class="mt-4 space-y-3 text-sm">
      <li v-for="item in items" :key="item.label">
        <RouterLink v-if="item.to" :to="item.to" class="text-fg hover:underline">
          {{ item.label }}
        </RouterLink>
        <a v-else :href="item.href" target="_blank" rel="noopener" class="text-fg hover:underline">
          {{ item.label }}
        </a>
      </li>
    </ul>
  </div>
</template>
