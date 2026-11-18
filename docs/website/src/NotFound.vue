<script setup lang="ts">
import { RouterLink } from 'vue-router'
</script>

<template>
  <div class="container-page py-24">
    <h1 class="font-display text-3xl font-bold">Page not found</h1>
    <p class="mt-4 text-muted">
      Go to the <RouterLink to="/docs" class="underline">documentation</RouterLink> or the
      <RouterLink to="/" class="underline">home page</RouterLink>.
    </p>
  </div>
</template>
