<script setup lang="ts">
import { computed, ref } from 'vue'

import CodeTabs from './code/CodeTabs.vue'
import CodeBlock from './code/CodeBlock.vue'

// Raw code imports
import CInstall from '../../examples/c/install.sh?raw'
import CCode from '../../examples/c/main.c?raw'
import PythonInstall from '../../examples/python/install.sh?raw'
import PythonCode from '../../examples/python/main.py?raw'
import RustInstall from '../../examples/rust/install.sh?raw'
import RustCode from '../../examples/rust/main.rs?raw'

interface Example {
  id: string
  label: string
  install: string
  code: string
}

const examples: Example[] = [
  { id: 'python', label: 'Python', install: PythonInstall, code: PythonCode },
  { id: 'c', label: 'C', install: CInstall, code: CCode },
  { id: 'rust', label: 'Rust', install: RustInstall, code: RustCode },
]

const activeLang = ref(examples[0].id)

const currentExample = computed(
  () => examples.find((e) => e.id === activeLang.value) ?? examples[0],
)
</script>

<template>
  <section class="border-y border-border bg-surface/30 py-20 md:py-28">
    <div class="container-page">
      <div class="max-w-2xl">
        <div class="eyebrow">Quickstart</div>
        <h2 class="mt-4 font-display text-3xl font-bold tracking-tight sm:text-4xl md:text-5xl">
          Start in a few lines
        </h2>
      </div>

      <div class="mt-10 overflow-hidden rounded-xl border border-border bg-bg shadow-2xl shadow-black/40">
        <CodeTabs :items="examples" v-model="activeLang" />

        <div class="grid gap-px bg-border">
          <CodeBlock label="Setup" language="bash" :code="currentExample.install" />
          <CodeBlock label="Code" :language="currentExample.id" :code="currentExample.code" copyable />
        </div>
      </div>
    </div>
  </section>
</template>
