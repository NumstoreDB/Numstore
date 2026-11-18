<script setup lang="ts">
defineProps<{
  items: { id: string; label: string }[]
  modelValue: string
}>()
defineEmits<{ 'update:modelValue': [id: string] }>()
</script>

<template>
  <div class="border-b border-border bg-surface/40 px-2" role="tablist" aria-label="Language">
    <div class="flex flex-wrap">
      <button
        v-for="item in items"
        :key="item.id"
        type="button"
        role="tab"
        :aria-selected="modelValue === item.id"
        :tabindex="modelValue === item.id ? 0 : -1"
        class="relative -mb-px px-4 py-3 text-sm font-medium transition-colors min-h-[44px]"
        :class="modelValue === item.id
          ? 'text-fg border-b-2 border-accent-soft'
          : 'text-muted hover:text-fg border-b-2 border-transparent'"
        @click="$emit('update:modelValue', item.id)"
      >
        {{ item.label }}
      </button>
    </div>
  </div>
</template>
