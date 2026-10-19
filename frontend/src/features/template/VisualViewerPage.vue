<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ExternalLink, PanelRightClose, PanelRightOpen } from 'lucide-vue-next'
import { NAlert, NButton, NEmpty, NSelect, NSpin } from 'naive-ui'
import CodeView from '@/shared/components/CodeView.vue'
import MarkdownView from '@/shared/components/MarkdownView.vue'
import VisualFrame from '@/features/template/components/VisualFrame.vue'
import { fetchTemplateDetail } from '@/features/template/api'
import {
  parseVersionQuery,
  visualIframeSrc,
  visualVariants,
  visualVersionToken,
} from '@/features/template/model/visual'
import { ROOT_VERSION_TOKEN } from '@/features/template/types'
import type { LangId } from '@/shared/types'
import type { TemplateDetail, TemplateVariant } from '@/features/template/types'

const route = useRoute()
const router = useRouter()

const loading = ref(false)
const loadError = ref<string | null>(null)
const visualError = ref<string | null>(null)
const probing = ref(false)
const detail = ref<TemplateDetail | null>(null)
const sideOpen = ref(true)
const iframeReady = ref(false)

const category = computed(() => String(route.params.category ?? ''))
const name = computed(() => String(route.params.name ?? ''))
const templateId = computed(() => `${category.value}/${name.value}`)

const queryVersion = computed(() => parseVersionQuery(route.query.version))

const visualOnly = computed(() => (detail.value ? visualVariants(detail.value.variants) : []))

const selected = computed<TemplateVariant | null>(() => {
  const d = detail.value
  if (!d?.variants.length) return null
  const q = queryVersion.value
  if (q) {
    const found =
      q === ROOT_VERSION_TOKEN
        ? d.variants.find((v) => v.id === d.id)
        : d.variants.find((v) => v.name === q || v.id === `${d.id}/${q}`)
    if (found) return found
  }
  return d.variants[0] ?? null
})

const selectedHasVisual = computed(() => selected.value?.has_visual === true)

const versionOptions = computed(() =>
  visualOnly.value.map((v) => ({
    label: v.name,
    value: visualVersionToken(templateId.value, v),
  })),
)

const iframeSrc = computed(() => {
  if (!selected.value || !selectedHasVisual.value) return ''
  return visualIframeSrc(
    category.value,
    name.value,
    visualVersionToken(templateId.value, selected.value),
  )
})

const selectValue = computed(() => {
  if (!selected.value) return null
  return visualVersionToken(templateId.value, selected.value)
})

async function loadDetail(): Promise<void> {
  loading.value = true
  loadError.value = null
  detail.value = null
  try {
    detail.value = await fetchTemplateDetail(templateId.value)
  } catch (err) {
    loadError.value = err instanceof Error ? err.message : '模板不存在'
  } finally {
    loading.value = false
  }
}

async function probeVisual(): Promise<void> {
  iframeReady.value = false
  visualError.value = null
  const src = iframeSrc.value
  if (!src) {
    if (selected.value && !selectedHasVisual.value) {
      visualError.value = '当前版本没有 visual.html'
    }
    return
  }
  probing.value = true
  try {
    const resp = await fetch(src, { method: 'GET' })
    if (resp.status === 404) {
      visualError.value = '未找到可视化文件'
      return
    }
    if (resp.status === 413) {
      visualError.value = '可视化文件过大，无法加载'
      return
    }
    if (!resp.ok) {
      visualError.value = `加载失败（${resp.status}）`
      return
    }
    iframeReady.value = true
  } catch {
    visualError.value = '加载失败，请确认后端已启动'
  } finally {
    probing.value = false
  }
}

function onVersionChange(token: string | null): void {
  if (!token) return
  const d = detail.value
  if (!d?.variants.length) return
  const primary = visualVersionToken(templateId.value, d.variants[0])
  const query: Record<string, string> = {}
  if (token !== primary) query.version = token
  void router.replace({ query })
}

function openBlank(): void {
  if (iframeSrc.value) window.open(iframeSrc.value, '_blank', 'noopener')
}

watch(
  () => [category.value, name.value],
  () => {
    void loadDetail()
  },
  { immediate: true },
)

watch([detail, iframeSrc], () => {
  void probeVisual()
})
</script>

<template>
  <div class="vis-viewer" :class="{ 'side-collapsed': !sideOpen }">
    <n-spin :show="loading || probing">
      <n-alert v-if="loadError" type="error" :bordered="false">{{ loadError }}</n-alert>
      <div v-else class="vis-stage">
        <div class="vis-main">
          <div class="vis-main-bar">
            <n-select
              v-if="versionOptions.length > 1"
              :value="selectValue"
              :options="versionOptions"
              size="small"
              class="vis-ver"
              @update:value="onVersionChange"
            />
            <n-button size="small" quaternary :disabled="!iframeSrc" @click="openBlank">
              <template #icon><ExternalLink :size="14" /></template>
              新窗口打开
            </n-button>
            <n-button size="small" quaternary @click="sideOpen = !sideOpen">
              <template #icon>
                <PanelRightClose v-if="sideOpen" :size="14" />
                <PanelRightOpen v-else :size="14" />
              </template>
              {{ sideOpen ? '收起代码' : '显示代码' }}
            </n-button>
          </div>
          <n-alert v-if="visualError" type="warning" :bordered="false">{{ visualError }}</n-alert>
          <div v-else-if="iframeReady && iframeSrc" class="vis-frame-wrap">
            <VisualFrame :src="iframeSrc" />
          </div>
          <n-empty v-else-if="!probing && !selected" description="该模板没有版本" />
        </div>
        <aside v-if="sideOpen && selected" class="vis-side">
          <p class="vis-side-title">{{ selected.name }}</p>
          <CodeView :code="selected.code" :file="selected.file" :lang="(selected.lang as LangId)" />
          <div v-if="selected.body" class="detail-desc">
            <MarkdownView :content="selected.body" />
          </div>
        </aside>
      </div>
    </n-spin>
  </div>
</template>

<style scoped>
.vis-viewer {
  flex: 1;
  min-height: 0;
  display: flex;
  flex-direction: column;
  padding: 8px 16px 16px;
}

.vis-stage {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(240px, 340px);
  gap: 12px;
  min-height: 0;
  height: calc(100vh - 120px);
}

.vis-viewer.side-collapsed .vis-stage {
  grid-template-columns: minmax(0, 1fr);
}

.vis-main,
.vis-side {
  min-height: 0;
  min-width: 0;
  display: flex;
  flex-direction: column;
  border: 1px solid var(--border);
  border-radius: var(--radius);
  background: var(--surface);
  overflow: auto;
}

.vis-main-bar {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
  padding: 8px;
  border-bottom: 1px solid var(--border);
}

.vis-ver {
  width: 180px;
}

.vis-frame-wrap {
  flex: 1;
  min-height: 0;
}

.vis-side {
  padding: 10px;
}

.vis-side-title {
  margin: 0 0 8px;
  font-weight: 650;
}

@media (max-width: 900px) {
  .vis-stage {
    grid-template-columns: 1fr;
    height: auto;
  }
}
</style>
