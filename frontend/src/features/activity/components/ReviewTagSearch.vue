<script setup lang="ts">
/** 按算法查找面板：平台 → 标签 → 题库结果 → 一键收录。
 *
 * 平台间标签体系互不相通，所以必须先定平台（「全部」视图下默认要求先选一个）；
 * 标签目录每次打开都重取（不在前端缓存，避免标签属于 A 平台却显示在 B 平台）。
 * 结果行的「已通过 / 已收录」由后端用本地数据标好，前端只据此禁用按钮。
 * 收录成功后 emit added，由页面把条目并入列表；同时把该行标成已收录。 */

import { computed, ref, watch } from 'vue'
import { ExternalLink, Search } from 'lucide-vue-next'
import {
  NButton,
  NEmpty,
  NModal,
  NPagination,
  NSelect,
  NSpin,
  NTag,
  useMessage,
} from 'naive-ui'
import {
  addReviewManual,
  fetchReviewTags,
  searchReviewProblems,
} from '@/features/activity/api'
import { difficultyLabel, supportsTagSearch } from '@/features/activity/model/review'
import { useActivity, type PlatformScope } from '@/features/activity/store'
import type {
  PlatformId,
  ProblemTag,
  ReviewItem,
  SearchProblem,
} from '@/features/activity/types'

const props = defineProps<{
  show: boolean
  /** 打开时的默认平台（当前筛选的平台；「全部」时为空） */
  initialPlatform: PlatformScope
}>()

const emit = defineEmits<{
  'update:show': [value: boolean]
  /** 收录成功（页面据此并入列表） */
  added: [item: ReviewItem, created: boolean]
}>()

const { platforms, platformMeta } = useActivity()
const message = useMessage()

/** 只列具备按算法查找能力的平台（与后端 review_tags 的能力分支一致） */
const platformOptions = computed(() =>
  platforms.value
    .filter((p) => supportsTagSearch(p.capabilities))
    .map((p) => ({ label: p.name, value: p.id })),
)

const platform = ref<PlatformId | null>(null)
const tags = ref<ProblemTag[]>([])
const tag = ref<string | null>(null)
const tagLoading = ref(false)

const items = ref<SearchProblem[]>([])
const page = ref(1)
const hasMore = ref(false)
const searching = ref(false)
const errorText = ref('')

/** 正在收录的题目键（防重复点击与并发写） */
const collecting = ref<string[]>([])

const tagOptions = computed(() =>
  tags.value.map((t) => ({
    label: t.count === null ? t.name : `${t.name}（${t.count}）`,
    value: t.id,
    group: t.group || undefined,
  })),
)

const canSearch = computed(() => platform.value !== null && tag.value !== null)

function reset(): void {
  tags.value = []
  tag.value = null
  items.value = []
  page.value = 1
  hasMore.value = false
  errorText.value = ''
}

async function loadTags(): Promise<void> {
  if (!platform.value) return
  tagLoading.value = true
  errorText.value = ''
  try {
    const res = await fetchReviewTags(platform.value)
    tags.value = res.items
  } catch (e) {
    tags.value = []
    errorText.value = e instanceof Error ? e.message : '标签目录加载失败'
  } finally {
    tagLoading.value = false
  }
}

async function search(target: number): Promise<void> {
  if (!platform.value || !tag.value) return
  searching.value = true
  errorText.value = ''
  try {
    const res = await searchReviewProblems(platform.value, tag.value, target)
    items.value = res.items
    page.value = res.page
    hasMore.value = res.hasMore
  } catch (e) {
    items.value = []
    hasMore.value = false
    errorText.value = e instanceof Error ? e.message : '题库检索失败'
  } finally {
    searching.value = false
  }
}

/* 打开面板：从默认平台重新开始（不保留上次的标签与结果） */
watch(
  () => props.show,
  (show) => {
    if (!show) return
    reset()
    const preferred = props.initialPlatform
    const usable =
      preferred !== 'all' &&
      supportsTagSearch(platformMeta(preferred)?.capabilities ?? [])
    platform.value = usable
      ? preferred
      : ((platformOptions.value[0]?.value as PlatformId | undefined) ?? null)
    if (platform.value) void loadTags()
  },
)

/* 换平台：标签体系不同，清空标签与结果后重取目录 */
watch(platform, (now, before) => {
  if (now === before) return
  reset()
  if (now) void loadTags()
})

/* 换标签：从第 1 页重新检索 */
watch(tag, (now, before) => {
  if (now === before || now === null) return
  void search(1)
})

function isCollecting(problem: SearchProblem): boolean {
  return collecting.value.includes(problem.problemKey)
}

async function collect(problem: SearchProblem): Promise<void> {
  if (!platform.value || problem.solved || problem.collected) return
  if (isCollecting(problem)) return
  collecting.value = [...collecting.value, problem.problemKey]
  try {
    const res = await addReviewManual({
      platform: platform.value,
      problemKey: problem.problemKey,
    })
    // 本地标注已收录：按钮立即禁用，不必重查整页
    items.value = items.value.map((i) =>
      i.problemKey === problem.problemKey ? { ...i, collected: true } : i,
    )
    emit('added', res.item, res.created)
    message.success(res.created ? '已加入错题本' : '该题已在错题本中')
  } catch (e) {
    message.error(e instanceof Error ? e.message : '收录失败，请稍后重试')
  } finally {
    collecting.value = collecting.value.filter((k) => k !== problem.problemKey)
  }
}

function actionLabel(problem: SearchProblem): string {
  if (problem.solved) return '已通过'
  if (problem.collected) return '已收录'
  return '收录'
}
</script>

<template>
  <NModal
    :show="show"
    preset="card"
    title="按算法查找题目"
    class="tag-search-modal"
    :style="{ width: 'min(760px, calc(100vw - 40px))' }"
    @update:show="emit('update:show', $event)"
  >
    <div class="search-bar">
      <NSelect
        v-model:value="platform"
        class="platform-select"
        size="small"
        placeholder="选择平台"
        :options="platformOptions"
      />
      <NSelect
        v-model:value="tag"
        class="tag-select"
        size="small"
        filterable
        clearable
        :loading="tagLoading"
        :disabled="!platform"
        :options="tagOptions"
        :placeholder="platform ? '选择算法标签' : '先选平台（标签体系各平台不同）'"
      />
    </div>

    <p v-if="errorText" class="search-hint error">{{ errorText }}</p>

    <div v-if="searching" class="search-state">
      <NSpin size="small" />
    </div>
    <NEmpty
      v-else-if="items.length === 0"
      class="search-state"
      :description="canSearch ? '该标签下没有更多题目' : '选一个算法标签开始查找'"
    />
    <ul v-else class="search-list">
      <li v-for="problem in items" :key="problem.problemKey" class="search-row">
        <span class="row-key">{{ problem.problemKey }}</span>
        <a
          class="row-name"
          :href="problem.url"
          target="_blank"
          rel="noopener noreferrer"
        >
          {{ problem.name }}
          <ExternalLink :size="11" />
        </a>
        <span class="row-rating">{{ difficultyLabel(problem.rating) }}</span>
        <span class="row-tags">{{ problem.tags.slice(0, 2).join(' · ') }}</span>
        <NTag v-if="problem.solved" size="small" :bordered="false" type="success">
          已通过
        </NTag>
        <NButton
          v-else
          size="tiny"
          type="primary"
          secondary
          :loading="isCollecting(problem)"
          :disabled="problem.collected"
          @click="collect(problem)"
        >
          {{ actionLabel(problem) }}
        </NButton>
      </li>
    </ul>

    <div v-if="items.length > 0 || page > 1" class="search-pager">
      <NPagination
        :page="page"
        :page-count="hasMore ? page + 1 : page"
        :page-slot="5"
        size="small"
        @update:page="search"
      />
    </div>

    <p class="search-foot">
      <Search :size="12" />
      每页 50 题；「已通过」来自你的提交，「已收录」来自手动清单，两者都不会重复加入。
    </p>
  </NModal>
</template>

<style scoped>
.search-bar {
  display: flex;
  align-items: center;
  gap: 8px;
}

.platform-select {
  flex: none;
  width: 150px;
}

.tag-select {
  flex: 1;
  min-width: 0;
}

.search-hint {
  margin: 8px 0 0;
  font-size: 12.5px;
}

.search-hint.error {
  color: #c63b57;
}

.search-state {
  display: flex;
  justify-content: center;
  padding: 36px 0;
}

.search-list {
  margin: 10px 0 0;
  padding: 0;
  list-style: none;
  display: flex;
  flex-direction: column;
  gap: 4px;
  max-height: 46vh;
  overflow-y: auto;
}

.search-row {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 7px 10px;
  border: 1px solid var(--border);
  border-radius: 8px;
  font-size: 12.5px;
}

.row-key {
  flex: none;
  min-width: 62px;
  font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-weight: 600;
  color: var(--text);
}

.row-name {
  flex: 1;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  display: inline-flex;
  align-items: center;
  gap: 4px;
  color: var(--text);
  text-decoration: none;
}

.row-name:hover {
  color: var(--accent-strong);
}

.row-rating {
  flex: none;
  min-width: 34px;
  text-align: right;
  color: var(--muted);
  font-variant-numeric: tabular-nums;
}

.row-tags {
  flex: none;
  max-width: 180px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  color: var(--faint);
}

.search-pager {
  display: flex;
  justify-content: center;
  padding: 10px 0 0;
}

.search-foot {
  display: flex;
  align-items: center;
  gap: 5px;
  margin: 10px 0 0;
  font-size: 11.5px;
  color: var(--faint);
}
</style>
