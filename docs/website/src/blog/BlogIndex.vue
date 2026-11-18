<script setup lang="ts">
import { RouterLink } from 'vue-router'
import { posts } from 'virtual:content'
import { formatDate } from './date'
</script>

<template>
  <div class="container-page max-w-3xl py-10">
    <h1 class="font-display text-3xl font-bold sm:text-4xl">Blog</h1>

    <p v-if="posts.length === 0" class="mt-8 text-muted">No posts yet.</p>

    <ul v-else class="mt-8 divide-y divide-border border-y border-border">
      <li v-for="post in posts" :key="post.path">
        <RouterLink :to="post.path" class="block py-6 hover:bg-elevated sm:px-4">
          <time :datetime="post.date!" class="text-xs text-muted">{{
            formatDate(post.date!)
          }}</time>
          <h2 class="mt-1 text-xl font-semibold">{{ post.title }}</h2>
          <p v-if="post.excerpt" class="mt-2 text-sm text-muted">{{ post.excerpt }}</p>
        </RouterLink>
      </li>
    </ul>
  </div>
</template>
