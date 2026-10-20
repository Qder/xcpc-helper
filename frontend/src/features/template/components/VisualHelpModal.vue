<script setup lang="ts">
import { NButton, NModal } from 'naive-ui'

defineProps<{
  show: boolean
}>()

const emit = defineEmits<{
  'update:show': [value: boolean]
}>()
</script>

<template>
  <n-modal
    :show="show"
    preset="card"
    title="使用说明"
    class="create-modal"
    :style="{ width: 'min(560px, calc(100vw - 40px))' }"
    @update:show="emit('update:show', $event)"
  >
    <div class="vis-help">
      <p>每个模板版本都可以有一份自包含的 <code>visual.html</code>，放在代码和 README 旁边。有这份文件的模板会出现在本画廊。</p>
      <ol>
        <li>
          <b>放到版本目录</b>
          ：在 <code>backend/content/分类/模板/版本/</code> 下放入 <code>visual.html</code>（顶层单版本则放在模板目录）。保存后稍等列表刷新。
        </li>
        <li>
          <b>在本页粘贴导入</b>
          ：点「导入可视化」，选模板和版本，把完整 HTML 粘进去。已有演示会先确认再覆盖。
        </li>
        <li>
          <b>让 AI 生成</b>
          ：新开对话，直接说「把某某算法可视化」。它会按仓库里的提示词生成，并写入对应版本目录。
        </li>
      </ol>
      <p class="vis-help-muted">
        骨架在 <code>docs/design/template/visual-skeleton.html</code>。文件必须内联全部 CSS/JS，不要引用外部资源。
      </p>
    </div>
    <div class="modal-actions">
      <n-button type="primary" @click="emit('update:show', false)">知道了</n-button>
    </div>
  </n-modal>
</template>

<style scoped>
.vis-help {
  display: flex;
  flex-direction: column;
  gap: 10px;
  font-size: 13px;
  line-height: 1.65;
}

.vis-help ol {
  margin: 0;
  padding-left: 1.25em;
}

.vis-help li + li {
  margin-top: 8px;
}

.vis-help-muted {
  margin: 0;
  color: var(--muted);
  font-size: 12px;
}
</style>
