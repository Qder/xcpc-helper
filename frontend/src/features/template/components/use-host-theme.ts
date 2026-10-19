/** 监听宿主 documentElement 的主题（data-theme 与 --hue），供可视化 iframe 同步。 */

import { onBeforeUnmount, onMounted, ref } from 'vue'

export function useHostTheme() {
  const dark = ref(false)
  const hue = ref(160)

  const refresh = (): void => {
    const el = document.documentElement
    dark.value = el.dataset.theme === 'dark'
    const hueRaw = getComputedStyle(el).getPropertyValue('--hue').trim()
    hue.value = hueRaw === '' ? 160 : Number(hueRaw)
  }

  let observer: MutationObserver | null = null

  onMounted(() => {
    refresh()
    observer = new MutationObserver(refresh)
    observer.observe(document.documentElement, {
      attributes: true,
      attributeFilter: ['data-theme', 'style'],
    })
  })

  onBeforeUnmount(() => observer?.disconnect())

  return { dark, hue, refresh }
}
