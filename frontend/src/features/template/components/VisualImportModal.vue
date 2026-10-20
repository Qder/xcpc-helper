<script setup lang="ts">
import { computed, ref, watch } from 'vue'
import { FileUp, Save } from 'lucide-vue-next'
import {
  NAlert,
  NButton,
  NForm,
  NFormItem,
  NInput,
  NModal,
  NSelect,
  useMessage,
  type SelectOption,
} from 'naive-ui'
import { ApiError } from '@/shared/api/client'
import { fetchTemplateDetail, fetchTemplates, putVisual } from '@/features/template/api'
import { splitTemplateId, visualVersionToken } from '@/features/template/model/visual'
import type { TemplateDetail, TemplateSummary, TemplateVariant } from '@/features/template/types'

const props = defineProps<{
  show: boolean
}>()

const emit = defineEmits<{
  'update:show': [value: boolean]
  imported: [category: string, name: string, versionToken: string]
}>()

const message = useMessage()
const submitting = ref(false)
const loadingList = ref(false)
const loadingDetail = ref(false)
const templates = ref<TemplateSummary[]>([])
const detail = ref<TemplateDetail | null>(null)
const templateId = ref<string | null>(null)
const versionToken = ref<string | null>(null)
const html = ref('')
const confirmOverwrite = ref(false)
const fileInput = ref<HTMLInputElement | null>(null)

const templateOptions = computed<SelectOption[]>(() =>
  templates.value
    .filter((t) => t.variant_count > 0)
    .map((t) => ({ label: `${t.cat} / ${t.name}`, value: t.id })),
)

const versionOptions = computed<SelectOption[]>(() => {
  const d = detail.value
  if (!d) return []
  return d.variants.map((v) => ({
    label: v.has_visual ? `${v.name}（已有演示）` : v.name,
    value: visualVersionToken(d.id, v),
  }))
})

const selectedVariant = computed<TemplateVariant | null>(() => {
  const d = detail.value
  const token = versionToken.value
  if (!d || !token) return null
  return d.variants.find((v) => visualVersionToken(d.id, v) === token) ?? null
})

const selectedHasVisual = computed(() => selectedVariant.value?.has_visual === true)

async function loadTemplates(): Promise<void> {
  loadingList.value = true
  try {
    templates.value = await fetchTemplates({ sort: 'name' })
  } catch (err) {
    message.error(err instanceof Error ? err.message : '加载模板列表失败')
  } finally {
    loadingList.value = false
  }
}

watch(
  () => props.show,
  (open) => {
    if (!open) return
    templateId.value = null
    versionToken.value = null
    detail.value = null
    html.value = ''
    confirmOverwrite.value = false
    void loadTemplates()
  },
)

watch(templateId, async (id) => {
  versionToken.value = null
  detail.value = null
  confirmOverwrite.value = false
  if (!id) return
  loadingDetail.value = true
  try {
    const next = await fetchTemplateDetail(id)
    detail.value = next
    if (next.variants[0]) {
      versionToken.value = visualVersionToken(next.id, next.variants[0])
    }
  } catch (err) {
    message.error(err instanceof Error ? err.message : '加载版本失败')
  } finally {
    loadingDetail.value = false
  }
})

watch(versionToken, () => {
  confirmOverwrite.value = false
})

function onFilePicked(event: Event): void {
  const input = event.target as HTMLInputElement
  const file = input.files?.[0]
  input.value = ''
  if (!file) return
  const reader = new FileReader()
  reader.onload = () => {
    html.value = typeof reader.result === 'string' ? reader.result : ''
  }
  reader.readAsText(file, 'utf-8')
}

async function submit(): Promise<void> {
  if (!templateId.value || !versionToken.value) {
    message.warning('请选择模板和版本')
    return
  }
  if (!html.value.trim()) {
    message.warning('请粘贴 visual.html 源码')
    return
  }
  if (selectedHasVisual.value && !confirmOverwrite.value) {
    confirmOverwrite.value = true
    return
  }
  submitting.value = true
  try {
    const { category, name } = splitTemplateId(templateId.value)
    await putVisual(category, name, versionToken.value, html.value)
    message.success('已写入 visual.html')
    emit('imported', category, name, versionToken.value)
    emit('update:show', false)
  } catch (err) {
    const text = err instanceof ApiError ? err.message : err instanceof Error ? err.message : '导入失败'
    message.error(text)
  } finally {
    submitting.value = false
  }
}
</script>

<template>
  <n-modal
    :show="show"
    preset="card"
    title="导入可视化"
    class="create-modal"
    :style="{ width: 'min(640px, calc(100vw - 40px))' }"
    @update:show="emit('update:show', $event)"
  >
    <n-form label-placement="top" size="small">
      <n-form-item label="模板">
        <n-select
          v-model:value="templateId"
          :options="templateOptions"
          :loading="loadingList"
          filterable
          placeholder="选择已有模板（需至少有一个版本）"
        />
      </n-form-item>
      <n-form-item label="版本">
        <n-select
          v-model:value="versionToken"
          :options="versionOptions"
          :loading="loadingDetail"
          :disabled="!templateId"
          placeholder="选择版本"
        />
      </n-form-item>
      <n-form-item label="visual.html 源码">
        <div class="vis-import-html">
          <n-button size="small" @click="fileInput?.click()">
            <template #icon><FileUp :size="14" /></template>
            选择 .html 文件
          </n-button>
          <input ref="fileInput" type="file" hidden accept=".html,.htm,text/html" @change="onFilePicked" />
          <n-input
            v-model:value="html"
            type="textarea"
            :rows="10"
            class="modal-code-field"
            placeholder="把 AI 生成的完整 HTML（从 <!doctype html> 到 </html>）粘贴到这里"
          />
        </div>
      </n-form-item>
      <n-alert v-if="confirmOverwrite" type="warning" :bordered="false">
        该版本已有 visual.html，再次点击将覆盖，无法找回。
      </n-alert>
      <div class="modal-actions">
        <n-button @click="emit('update:show', false)">取消</n-button>
        <n-button type="primary" :loading="submitting" @click="submit">
          <template #icon><Save :size="15" /></template>
          {{ confirmOverwrite ? '确认覆盖' : selectedHasVisual ? '覆盖导入' : '导入' }}
        </n-button>
      </div>
    </n-form>
  </n-modal>
</template>

<style scoped>
.vis-import-html {
  display: flex;
  flex-direction: column;
  gap: 8px;
  width: 100%;
}
</style>
