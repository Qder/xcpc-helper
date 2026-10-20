<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import { FileUp, Play, Search } from 'lucide-vue-next'
import { NAlert, NButton, NEmpty, NInput, NSelect, NSpin } from 'naive-ui'
import VisualImportModal from '@/features/template/components/VisualImportModal.vue'
import { fetchCategories, fetchTemplates, type ApiCategory } from '@/features/template/api'
import { filterVisualSummaries, splitTemplateId, visualViewerPath } from '@/features/template/model/visual'
import type { TemplateSummary } from '@/features/template/types'

const router = useRouter()
const keyword = ref('')
const category = ref('all')
const loading = ref(false)
const errorText = ref<string | null>(null)
const items = ref<TemplateSummary[]>([])
const cats = ref<ApiCategory[]>([])
const showImport = ref(false)

const categoryOptions = computed(() => [
  { label: '全部分类', value: 'all' },
  ...cats.value.map((c) => ({ label: `${c.name}（${c.count}）`, value: c.id })),
])

const shown = computed(() => filterVisualSummaries(items.value, category.value, keyword.value))

async function load(): Promise<void> {
  loading.value = true
  errorText.value = null
  try {
    const [list, catList] = await Promise.all([
      fetchTemplates({ hasVisual: true, sort: 'priority' }),
      fetchCategories(),
    ])
    items.value = list
    cats.value = catList
  } catch (err) {
    errorText.value = err instanceof Error ? err.message : '加载失败'
  } finally {
    loading.value = false
  }
}

function openCard(item: TemplateSummary): void {
  const { category: cat, name } = splitTemplateId(item.id)
  void router.push(visualViewerPath(cat, name))
}

async function onImported(cat: string, name: string, versionToken: string): Promise<void> {
  await load()
  void router.push(visualViewerPath(cat, name, versionToken))
}

onMounted(() => {
  void load()
})
</script>

<template>
  <div class="vis-gallery">
    <div class="vis-toolbar">
      <n-input v-model:value="keyword" clearable placeholder="搜索模板名、分类或标签…">
        <template #prefix>
          <Search :size="15" />
        </template>
      </n-input>
      <n-select v-model:value="category" :options="categoryOptions" class="vis-cat" />
      <div class="vis-toolbar-actions">
        <n-button size="small" @click="showImport = true">
          <template #icon><FileUp :size="14" /></template>
          导入可视化
        </n-button>
      </div>
    </div>
    <n-spin :show="loading">
      <n-alert v-if="errorText" type="error" :bordered="false">{{ errorText }}</n-alert>
      <n-empty
        v-else-if="!shown.length && !loading"
        description="在版本目录放入 visual.html 即可出现在这里"
      >
        <template #extra>
          <p class="vis-empty-hint">
            骨架文件：docs/design/template/visual-skeleton.html（与代码、README.md 并排放在版本目录下）
          </p>
        </template>
      </n-empty>
      <div v-else class="vis-cards">
        <button
          v-for="item in shown"
          :key="item.id"
          type="button"
          class="vis-card"
          @click="openCard(item)"
        >
          <span class="vis-card-icon"><Play :size="16" /></span>
          <span class="vis-card-body">
            <span class="vis-card-name">{{ item.name }}</span>
            <span class="vis-card-meta">{{ item.cat }} · {{ item.variant_count }} 个版本</span>
            <span v-if="item.tags.length" class="vis-card-tags">
              <span v-for="tag in item.tags" :key="tag" class="tag">{{ tag }}</span>
            </span>
          </span>
        </button>
      </div>
    </n-spin>
    <VisualImportModal v-model:show="showImport" @imported="onImported" />
  </div>
</template>

<style scoped>
.vis-gallery {
  flex: 1;
  min-height: 0;
  display: flex;
  flex-direction: column;
  gap: 12px;
  padding: 12px 20px 20px;
}

.vis-toolbar {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
}

.vis-cat {
  width: 200px;
}

.vis-toolbar-actions {
  margin-left: auto;
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.vis-empty-hint {
  margin: 8px 0 0;
  font-size: 12px;
  color: var(--muted);
}

.vis-cards {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(240px, 1fr));
  gap: 10px;
}

.vis-card {
  display: flex;
  gap: 10px;
  text-align: left;
  padding: 12px;
  border: 1px solid var(--border);
  border-radius: var(--radius);
  background: var(--surface);
  color: var(--text);
  cursor: pointer;
  transition: background 0.16s ease, border-color 0.16s ease;
}

.vis-card:hover {
  background: var(--surface-2);
  border-color: var(--accent);
}

.vis-card-icon {
  flex: none;
  color: var(--accent);
}

.vis-card-body {
  display: flex;
  flex-direction: column;
  gap: 4px;
  min-width: 0;
}

.vis-card-name {
  font-weight: 650;
}

.vis-card-meta {
  font-size: 12px;
  color: var(--muted);
}

.vis-card-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
}
</style>
