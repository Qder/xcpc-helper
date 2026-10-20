<script setup lang="ts">
import SideNav from '@/app/SideNav.vue'
import TopBar from '@/app/TopBar.vue'
import type { ThemeMode } from '@/shared/composables/useTheme'

defineProps<{
  activePath: string
  openGroups: Record<string, boolean>
  pageMeta: { group: string; sub: string }
  navCollapsed: boolean
  mode: ThemeMode
  modeIcon: 'sun' | 'moon' | 'monitor'
  modeLabel: string
  hue: number
  isDark: boolean
}>()

const emit = defineEmits<{
  navigate: [path: string]
  toggle: [id: string]
  'toggle-nav': []
  'cycle-theme': []
  'set-mode': [mode: ThemeMode]
  'set-hue': [value: number]
}>()
</script>

<template>
  <div class="app-shell" :class="{ 'nav-collapsed': navCollapsed }">
    <SideNav
      :active-path="activePath"
      :open-groups="openGroups"
      @navigate="(path) => emit('navigate', path)"
      @toggle="(id) => emit('toggle', id)"
    />
    <main class="main-pane">
      <TopBar
        :page-meta="pageMeta"
        :nav-collapsed="navCollapsed"
        :mode="mode"
        :mode-icon="modeIcon"
        :mode-label="modeLabel"
        :hue="hue"
        :is-dark="isDark"
        @toggle-nav="emit('toggle-nav')"
        @cycle-theme="emit('cycle-theme')"
        @set-mode="(value) => emit('set-mode', value)"
        @set-hue="(value) => emit('set-hue', value)"
      />
      <section class="page-stage">
        <slot />
      </section>
    </main>
  </div>
</template>
