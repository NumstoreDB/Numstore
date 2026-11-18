<script setup lang="ts">
defineProps<{ siteName: string; version: string }>()
const year = new Date().getFullYear()
</script>

<template>
  <div class="border-t border-border">
    <div class="container-page flex flex-col items-start justify-between gap-2 py-6 text-xs text-muted md:flex-row md:items-center">
      <p>© {{ year }} {{ siteName }}</p>
      <p class="font-mono text-[11px]">{{ version }} · built with Vue + Vite</p>
    </div>
  </div>
</template>
