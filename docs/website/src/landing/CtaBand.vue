<script setup lang="ts">
import { RouterLink } from 'vue-router'
import { startPath } from '../content/nav'
import { site_data } from '../data'
</script>

<template>
  <section class="border-y border-border py-16">
    <div class="container-page">
      <div class="border border-border p-10 md:p-16">
        <div class="max-w-2xl">
          <h2 class="font-display text-3xl font-bold tracking-tight sm:text-4xl md:text-5xl">
            Start with Numstore
          </h2>
          <div class="mt-8 flex flex-wrap gap-3">
            <RouterLink :to="startPath" class="btn-primary">Read the docs</RouterLink>
            <a :href="`mailto:${site_data.contact.email}`" class="btn-secondary"> Contact </a>
          </div>
        </div>
      </div>
    </div>
  </section>
</template>
