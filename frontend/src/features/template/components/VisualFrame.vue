<script setup lang="ts">
import { nextTick, ref, watch } from 'vue'
import { buildThemeMessage } from '@/features/template/model/visual'
import { useHostTheme } from '@/features/template/components/use-host-theme'

const props = defineProps<{
  src: string
}>()

const frame = ref<HTMLIFrameElement | null>(null)
const { dark, hue } = useHostTheme()

function postTheme(): void {
  const win = frame.value?.contentWindow
  if (!win) return
  win.postMessage(buildThemeMessage(dark.value, hue.value), '*')
}

function onLoad(): void {
  void nextTick(postTheme)
}

watch([dark, hue], () => postTheme())
watch(
  () => props.src,
  () => {
    void nextTick(postTheme)
  },
)
</script>

<template>
  <iframe
    ref="frame"
    class="vis-frame"
    :src="src"
    title="算法可视化"
    sandbox="allow-scripts"
    referrerpolicy="no-referrer"
    @load="onLoad"
  ></iframe>
</template>

<style scoped>
.vis-frame {
  display: block;
  width: 100%;
  height: 100%;
  min-height: 360px;
  border: 0;
  background: var(--surface);
}
</style>
