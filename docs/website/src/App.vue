<script setup lang="ts">
import { RouterView } from 'vue-router'
import AppBar from './AppBar.vue'
import AppFooter from './footer/AppFooter.vue'
</script>

<template>
  <div class="flex min-h-screen flex-col bg-bg text-fg">
    <AppBar />

    <main class="flex-1">
      <RouterView v-slot="{ Component, route }">
        <transition name="page" mode="out-in">
          <component :is="Component" :key="route.path" />
        </transition>
      </RouterView>
    </main>

    <AppFooter />
  </div>
</template>

<style>
.page-enter-active,
.page-leave-active {
  transition: opacity 0.2s ease;
}
.page-enter-from,
.page-leave-to {
  opacity: 0;
}
</style>
