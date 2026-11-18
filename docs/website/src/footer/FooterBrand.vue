<script setup lang="ts">
defineProps<{ name: string; }>()
</script>

<template>
  <div class="flex items-center gap-2 font-display text-lg font-bold">
    <img src="/logo.png" class="h-7 w-7" aria-hidden="true" alt="logo" />
    {{ name }}
  </div>
</template>
