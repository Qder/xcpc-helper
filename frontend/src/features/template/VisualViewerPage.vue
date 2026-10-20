<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import {
  ExternalLink,
  PanelLeftClose,
  PanelLeftOpen,
  PanelRightClose,
  PanelRightOpen,
  RotateCcw,
  Trash2,
} from 'lucide-vue-next'
import { NAlert, NButton, NEmpty, NSelect, NSpin, useMessage } from 'naive-ui'
import { useNavCollapse } from '@/app/use-nav-collapse'
import CodeView from '@/shared/components/CodeView.vue'
import DeleteConfirmModal from '@/shared/components/DeleteConfirmModal.vue'
import MarkdownView from '@/shared/components/MarkdownView.vue'
import VisualFrame from '@/features/template/components/VisualFrame.vue'
import {
  clearVisualHistory,
  deleteVisualHistoryItem,
  fetchTemplateDetail,
  fetchVisualHistory,
  restoreVisualHistory,
  type VisualHistoryItem,
} from '@/features/template/api'
import { ApiError } from '@/shared/api/client'
import {
  formatHistorySavedAt,
  parseHistoryQuery,
  parseVersionQuery,
  visualHistoryIframeSrc,
  visualIframeSrc,
  visualVariants,
  visualVersionToken,
} from '@/features/template/model/visual'
import { ROOT_VERSION_TOKEN } from '@/features/template/types'
import type { LangId } from '@/shared/types'
import type { TemplateDetail, TemplateVariant } from '@/features/template/types'

const route = useRoute()
const router = useRouter()
const message = useMessage()
const { navCollapsed, toggleNav } = useNavCollapse()

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
const queryHistory = computed(() => parseHistoryQuery(route.query.history))
const historyItems = ref<VisualHistoryItem[]>([])
const historyLoading = ref(false)
const mutating = ref(false)
const confirmKind = ref<'one' | 'all' | null>(null)

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

const versionToken = computed(() =>
  selected.value ? visualVersionToken(templateId.value, selected.value) : null,
)

const iframeSrc = computed(() => {
  if (!selected.value || !selectedHasVisual.value || !versionToken.value) return ''
  if (queryHistory.value) {
    return visualHistoryIframeSrc(
      category.value,
      name.value,
      versionToken.value,
      queryHistory.value,
    )
  }
  return visualIframeSrc(category.value, name.value, versionToken.value)
})

const historyOptions = computed(() => [
  { label: '当前演示', value: '' },
  ...historyItems.value.map((item) => ({
    label: formatHistorySavedAt(item.saved_at),
    value: item.id,
  })),
])

const historySelect = computed(() => queryHistory.value ?? '')

const viewingHistory = computed(() => queryHistory.value !== null)

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

function replaceQuery(token: string | null, historyId: string | null): void {
  const d = detail.value
  const query: Record<string, string> = {}
  if (token && d?.variants.length) {
    const primary = visualVersionToken(templateId.value, d.variants[0])
    if (token !== primary) query.version = token
  }
  if (historyId) query.history = historyId
  void router.replace({ query })
}

function onVersionChange(token: string | null): void {
  if (!token) return
  replaceQuery(token, null)
}

function onHistoryChange(id: string | null): void {
  replaceQuery(versionToken.value, id || null)
}

async function loadHistory(): Promise<void> {
  historyItems.value = []
  if (!versionToken.value || !selectedHasVisual.value) return
  historyLoading.value = true
  try {
    const data = await fetchVisualHistory(category.value, name.value, versionToken.value)
    historyItems.value = data.items
  } catch {
    historyItems.value = []
  } finally {
    historyLoading.value = false
  }
}

function errText(err: unknown, fallback: string): string {
  if (err instanceof ApiError) return err.message
  if (err instanceof Error) return err.message
  return fallback
}

async function restoreCurrent(): Promise<void> {
  if (!versionToken.value || !queryHistory.value) return
  mutating.value = true
  try {
    await restoreVisualHistory(
      category.value,
      name.value,
      versionToken.value,
      queryHistory.value,
    )
    message.success('已恢复为当前演示')
    replaceQuery(versionToken.value, null)
    await loadHistory()
  } catch (err) {
    message.error(errText(err, '恢复失败'))
  } finally {
    mutating.value = false
  }
}

async function onConfirmDelete(): Promise<void> {
  if (!versionToken.value || !confirmKind.value) return
  mutating.value = true
  try {
    if (confirmKind.value === 'all') {
      await clearVisualHistory(category.value, name.value, versionToken.value)
      message.success('已清空历史')
      replaceQuery(versionToken.value, null)
    } else if (queryHistory.value) {
      await deleteVisualHistoryItem(
        category.value,
        name.value,
        versionToken.value,
        queryHistory.value,
      )
      message.success('已删除该快照')
      replaceQuery(versionToken.value, null)
    }
    confirmKind.value = null
    await loadHistory()
  } catch (err) {
    message.error(errText(err, '删除失败'))
  } finally {
    mutating.value = false
  }
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

watch(versionToken, () => {
  void loadHistory()
})
</script>

<template>
  <div class="vis-viewer" :class="{ 'side-collapsed': !sideOpen }">
    <n-spin :show="loading || probing">
      <n-alert v-if="loadError" type="error" :bordered="false">{{ loadError }}</n-alert>
      <div v-else class="vis-layout">
        <div class="vis-main-bar">
          <n-select
            v-if="versionOptions.length > 1"
            :value="selectValue"
            :options="versionOptions"
            size="small"
            class="vis-ver"
            @update:value="onVersionChange"
          />
          <n-select
            v-if="selectedHasVisual"
            :value="historySelect"
            :options="historyOptions"
            :loading="historyLoading"
            size="small"
            placeholder="历史"
            class="vis-hist"
            @update:value="onHistoryChange"
          />
          <n-button
            v-if="viewingHistory"
            size="small"
            :disabled="mutating"
            @click="restoreCurrent"
          >
            <template #icon><RotateCcw :size="14" /></template>
            恢复为当前
          </n-button>
          <n-button
            v-if="viewingHistory"
            size="small"
            quaternary
            :disabled="mutating"
            @click="confirmKind = 'one'"
          >
            <template #icon><Trash2 :size="14" /></template>
            删除此历史
          </n-button>
          <n-button
            v-if="selectedHasVisual && historyItems.length"
            size="small"
            quaternary
            :disabled="mutating"
            @click="confirmKind = 'all'"
          >
            清空历史
          </n-button>
          <n-button size="small" quaternary :disabled="!iframeSrc" @click="openBlank">
            <template #icon><ExternalLink :size="14" /></template>
            新窗口打开
          </n-button>
          <n-button size="small" quaternary @click="toggleNav">
            <template #icon>
              <PanelLeftClose v-if="!navCollapsed" :size="14" />
              <PanelLeftOpen v-else :size="14" />
            </template>
            {{ navCollapsed ? '显示导航' : '收起导航' }}
          </n-button>
          <n-button size="small" quaternary @click="sideOpen = !sideOpen">
            <template #icon>
              <PanelRightClose v-if="sideOpen" :size="14" />
              <PanelRightOpen v-else :size="14" />
            </template>
            {{ sideOpen ? '收起代码' : '显示代码' }}
          </n-button>
        </div>
        <div class="vis-stage">
          <div class="vis-main">
            <n-alert v-if="visualError" type="warning" :bordered="false">{{ visualError }}</n-alert>
            <div v-else-if="iframeReady && iframeSrc" class="vis-frame-wrap">
              <VisualFrame :src="iframeSrc" />
            </div>
            <n-empty v-else-if="!probing && !selected" description="该模板没有版本" />
          </div>
          <aside v-show="sideOpen && selected" class="vis-side">
            <p class="vis-side-title">{{ selected?.name }}</p>
            <CodeView
              v-if="selected"
              :code="selected.code"
              :file="selected.file"
              :lang="(selected.lang as LangId)"
            />
            <div v-if="selected?.body" class="detail-desc">
              <MarkdownView :content="selected.body" />
            </div>
          </aside>
        </div>
      </div>
    </n-spin>
    <DeleteConfirmModal
      :show="confirmKind !== null"
      :title="confirmKind === 'all' ? '清空历史' : '删除历史快照'"
      :target="
        confirmKind === 'all'
          ? '该版本全部历史快照（不删当前演示）'
          : (queryHistory ?? '')
      "
      :loading="mutating"
      @update:show="confirmKind = $event ? confirmKind : null"
      @confirm="onConfirmDelete"
    />
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

.vis-viewer :deep(.n-spin-container),
.vis-viewer :deep(.n-spin-content) {
  flex: 1;
  min-height: 0;
  display: flex;
  flex-direction: column;
}

.vis-layout {
  display: flex;
  flex-direction: column;
  min-height: 0;
  height: calc(100vh - 120px);
}

.vis-main-bar {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
  padding: 0 0 8px;
}

.vis-stage {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(240px, 340px);
  gap: 12px;
  min-height: 0;
  flex: 1;
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

.vis-ver {
  width: 180px;
}

.vis-hist {
  width: 240px;
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
