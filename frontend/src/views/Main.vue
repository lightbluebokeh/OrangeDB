<template>
  <el-row :gutter="16" style="width:100%">
    <el-col :md="18">
      <el-card>
        <div slot="header">
          <span>SQL 语句</span>
        </div>
        <el-input
          v-model="statements"
          type="textarea"
          :autosize="{ minRows: 3, maxRows: 8 }"
        />
        <div style="text-align:right">
          <el-button
            style="margin-top:16px"
            size="medium"
            id="run-button"
            type="primary"
            >运行 <em class="el-icon-d-arrow-right" />
          </el-button>
        </div>
      </el-card>

      <el-card style="margin-top:20px">
        <div slot="header">
          <span>运行结果</span>
        </div>
        <el-table />
      </el-card>
    </el-col>

    <el-col :md="6">
      <el-card>
        <div slot="header">
          <span>数据库信息</span>
        </div>
        <span>还没写</span>
      </el-card>
    </el-col>
  </el-row>
</template>

<script lang="ts">
import Vue from 'vue';
export default Vue.extend({
  data() {
    return {
      statements: '',
      results: [],
    };
  },
});
</script>

<style lang="scss" scoped></style>
