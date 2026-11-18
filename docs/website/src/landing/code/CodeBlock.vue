<script setup lang="ts">
import { computed, ref } from 'vue'
import { highlightCode } from '../../lib'

const props = defineProps<{
  code: string
  language: string
  label: string
  copyable?: boolean
}>()

const highlighted = computed(() => highlightCode(props.code, props.language))

const copied = ref(false)
async function copyCode() {
  try {
    await navigator.clipboard.writeText(props.code)
    copied.value = true
    setTimeout(() => (copied.value = false), 1500)
  } catch {
    copied.value = false
  }
}
</script>

<template>
  <div class="bg-bg p-5">
    <div class="flex items-center justify-between gap-3">
      <span class="font-mono text-[10px] uppercase tracking-widest text-muted">{{ label }}</span>
      <button
        v-if="copyable"
        type="button"
        class="inline-flex h-8 items-center gap-1.5 rounded-md border border-border bg-surface px-2.5 text-xs font-medium text-muted transition-colors hover:text-fg"
        :aria-label="copied ? 'Copied' : 'Copy code'"
        @click="copyCode"
      >
        <svg v-if="!copied" viewBox="0 0 24 24" class="h-3.5 w-3.5" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
          <rect x="9" y="9" width="13" height="13" rx="2" />
          <path d="M5 15V5a2 2 0 0 1 2-2h10" />
        </svg>
        <svg v-else viewBox="0 0 24 24" class="h-3.5 w-3.5 text-accent-soft" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
          <path d="M20 6 9 17l-5-5" />
        </svg>
        {{ copied ? 'Copied' : 'Copy' }}
      </button>
    </div>
    <pre class="mt-2 overflow-x-auto font-mono text-[13px] leading-[1.6]"><code :class="['hljs', `language-${language}`]" v-html="highlighted" /></pre>
  </div>
</template>

<style scoped>
:deep(.hljs) {
  background: transparent;
  padding: 0;
  color: #e7e7ea;
}
</style>
