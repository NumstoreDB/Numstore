<script setup lang="ts">
import { onMounted, onBeforeUnmount, ref } from 'vue'
import { RouterLink, useRoute } from 'vue-router'
import { tabs } from './content/nav'
import { site_data } from './data'
import GitHubStars from './GitHubStars.vue'

// Make top app bar look different if you are
// lower on the page (by 12 pixels)
const scrolled = ref(false)
const route = useRoute()

// The docs tabs from nav.ts, then the blog.
const links = [...tabs.map((t) => ({ label: t.label, to: t.path })), { label: 'Blog', to: '/blog' }]
const mobileOpen = ref(false)

function onScroll() {
  scrolled.value = window.scrollY > 12
}

onMounted(() => {
  onScroll()
  window.addEventListener('scroll', onScroll, { passive: true })
})

onBeforeUnmount(() => {
  window.removeEventListener('scroll', onScroll)
})

function closeMobile() {
  mobileOpen.value = false
}
</script>

<template>
  <header
    class="sticky top-0 z-50 w-full border-b border-border bg-bg transition-all duration-200"
    :class="scrolled ? 'py-1' : 'py-2'"
  >
    <div class="container-page flex items-center gap-4" :class="scrolled ? 'h-12' : 'h-16'">
      <RouterLink
        to="/"
        class="flex items-center gap-2 font-display text-lg font-bold tracking-tight text-fg"
        @click="closeMobile"
      >
        <img src="/logo.png" class="h-7 w-7 shrink-0" aria-hidden="true" alt="Logo" />
        <span
          class="transition-all duration-200 origin-left"
          :class="scrolled ? 'scale-x-0 opacity-0 w-0 overflow-hidden' : 'scale-x-100 opacity-100'"
          >{{ site_data.name }}
        </span>
      </RouterLink>

      <div class="flex-1" />

      <nav class="hidden items-center gap-1 lg:flex" aria-label="Primary">
        <RouterLink
          v-for="link in links"
          :key="link.to"
          :to="link.to"
          class="px-3 py-2 text-sm font-medium transition-colors hover:bg-elevated hover:text-fg"
          :class="route.meta.tab === link.label ? 'text-fg' : 'text-muted'"
          :aria-current="route.meta.tab === link.label ? 'page' : undefined"
          >{{ link.label }}</RouterLink
        >
      </nav>

      <div class="hidden items-center gap-2 lg:flex">
        <GitHubStars />
      </div>

      <div class="hidden lg:block">
        <a :href="`mailto:${site_data.contact.email}`" class="btn-primary">Contact</a>
      </div>

      <a :href="`mailto:${site_data.contact.email}`" class="btn-primary lg:hidden">Contact</a>

      <button
        type="button"
        class="inline-flex h-11 w-11 items-center justify-center text-fg hover:bg-elevated lg:hidden"
        :aria-expanded="mobileOpen"
        aria-controls="mobile-menu"
        aria-label="Toggle menu"
        @click="mobileOpen = !mobileOpen"
      >
        <svg
          v-if="!mobileOpen"
          class="h-6 w-6"
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2"
          stroke-linecap="round"
          stroke-linejoin="round"
        >
          <path d="M3 6h18M3 12h18M3 18h18" />
        </svg>
        <svg
          v-else
          class="h-6 w-6"
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2"
          stroke-linecap="round"
          stroke-linejoin="round"
        >
          <path d="M6 6l12 12M6 18L18 6" />
        </svg>
      </button>
    </div>

    <transition
      enter-active-class="transition duration-200 ease-out"
      enter-from-class="-translate-y-2 opacity-0"
      enter-to-class="translate-y-0 opacity-100"
      leave-active-class="transition duration-150 ease-in"
      leave-from-class="translate-y-0 opacity-100"
      leave-to-class="-translate-y-2 opacity-0"
    >
      <div v-if="mobileOpen" id="mobile-menu" class="border-t border-border bg-bg lg:hidden">
        <div class="container-page flex flex-col gap-1 py-4">
          <RouterLink
            v-for="link in links"
            :key="link.to"
            :to="link.to"
            class="px-3 py-3 text-base font-medium hover:bg-elevated"
            :class="route.meta.tab === link.label ? 'text-fg' : 'text-muted'"
            @click="closeMobile"
            >{{ link.label }}</RouterLink
          >
          <div class="mt-2 flex items-center gap-2 px-1">
            <GitHubStars />
          </div>
        </div>
      </div>
    </transition>
  </header>
</template>
