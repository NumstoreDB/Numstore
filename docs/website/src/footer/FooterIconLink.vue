<script setup lang="ts">
defineProps<{ href: string; label: string; external?: boolean }>()
</script>

<template>
  <a 
    :href="href"
    v-bind="external ? { target: '_blank', rel: 'noopener' } : {}"
    class="inline-flex items-center gap-2 text-fg hover:underline"
  >
    <slot />
    {{ label }}
  </a>
</template>
