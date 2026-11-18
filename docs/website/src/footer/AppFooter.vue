<script setup lang="ts">
import { site_data, icons } from '../data'
import FooterBrand from './FooterBrand.vue'
import FooterLinkList from './FooterLinkList.vue'
import FooterIconLink from './FooterIconLink.vue'
import FooterBottom from './FooterBottom.vue'

const githubUrl = `https://github.com/${site_data.github.owner}/${site_data.github.repo}`
const youtubeUrl = `https://www.youtube.com/${site_data.youtube.handle}`

const siteLinks = [
  { label: 'Home', to: '/' },
  { label: 'Documentation', to: '/docs' },
  { label: 'Blog', to: '/blog' },
  { label: 'Contact', href: `mailto:${site_data.contact.email}` },
]
</script>

<template>
  <footer class="border-t border-border bg-surface">
    <div class="container-page grid gap-12 py-16 md:grid-cols-12">
      <div class="md:col-span-5">
        <FooterBrand :name="site_data.name" />
      </div>

      <div class="md:col-span-4">
        <FooterLinkList title="Site" :items="siteLinks" />
      </div>

      <div class="md:col-span-3">
        <h4 class="text-xs font-semibold uppercase tracking-widest text-muted">Connect</h4>
        <ul class="mt-4 space-y-3 text-sm">
          <li>
            <FooterIconLink :href="githubUrl" label="GitHub" external>
              <svg viewBox="0 0 24 24" class="h-4 w-4" fill="currentColor" aria-hidden="true">
                <path :d="icons.github" />
              </svg>
            </FooterIconLink>
          </li>
          <li>
            <FooterIconLink :href="site_data.linkedin" label="LinkedIn" external>
              <svg viewBox="0 0 24 24" class="h-4 w-4" fill="currentColor" aria-hidden="true">
                <path :d="icons.linkedin" />
              </svg>
            </FooterIconLink>
          </li>
          <li>
            <FooterIconLink :href="youtubeUrl" label="YouTube" external>
              <svg viewBox="0 0 24 24" class="h-4 w-4" fill="currentColor" aria-hidden="true">
                <path :d="icons.youtube" />
              </svg>
            </FooterIconLink>
          </li>
          <li>
            <FooterIconLink :href="`mailto:${site_data.contact.email}`" label="Email">
              <svg
                viewBox="0 0 24 24"
                class="h-4 w-4"
                fill="none"
                stroke="currentColor"
                stroke-width="2"
                stroke-linecap="round"
                stroke-linejoin="round"
                aria-hidden="true"
              >
                <rect x="3" y="5" width="18" height="14" rx="2" />
                <path d="m3 7 9 6 9-6" />
              </svg>
            </FooterIconLink>
          </li>
        </ul>
      </div>
    </div>

    <FooterBottom :site-name="site_data.name" version="v0.1.0" />
  </footer>
</template>
