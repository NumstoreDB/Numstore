<script setup lang="ts">
import { RouterLink } from 'vue-router'
import { startPath } from '../content/nav'
</script>

<template>
  <section class="relative overflow-hidden">
    <div class="container-page py-20 md:py-28 lg:py-32">
      <div class="max-w-3xl">
        <div class="eyebrow">v1.5.1</div>
        <h1
          class="mt-6 font-display text-4xl font-bold leading-[1.05] tracking-tight text-fg sm:text-5xl md:text-6xl lg:text-7xl"
        >
          Numstore
        </h1>
        <p class="mt-6 max-w-xl text-base leading-relaxed text-muted md:text-lg">
          Numstore is a
          <span class="text-fg font-medium">database for arrays.</span>
          Insert data into the middle of an array, or remove it. Every change is atomic and survives a crash.
        </p>

        <div class="mt-10 flex flex-wrap gap-3">
          <RouterLink :to="startPath" class="btn-primary">
            Read the docs
            <svg
              viewBox="0 0 20 20"
              class="h-4 w-4"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
              aria-hidden="true"
            >
              <path d="M5 10h10M11 6l4 4-4 4" stroke-linecap="round" stroke-linejoin="round" />
            </svg>
          </RouterLink>
        </div>
      </div>

      <figure class="mt-16 md:mt-20">
        <img
          class="block w-full border border-border"
          src="/p6_bar_time_vs_file_size_log.png"
          alt="Benchmark: Numstore write time against file size, on a log scale"
        />
        <figcaption class="mt-3 text-center text-xs text-muted">
          This plot compares two ways to add data to the middle of a file. The first way opens a
          hole in a standard file and writes the data. The second way uses Numstore, in purple.
          Numstore takes logarithmic time in the size of the file. For details, read the
          <a href="https://theolincke.com/blog/13_inner_inserts" class="underline text-fg"
            >blog post on inner inserts</a
          >.
        </figcaption>
      </figure>
    </div>
  </section>
</template>
