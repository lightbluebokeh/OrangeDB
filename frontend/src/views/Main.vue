<template>
  <el-row :gutter="16" style="width:100%">
    <el-col :xs="24" :sm="24" :md="18">
      <el-card shadow="hover" style="margin-bottom:20px">
        <div slot="header">
          <span>数据库查询</span>
        </div>
        <el-input
          v-model="statements"
          type="textarea"
          placeholder="在这里输入 SQL 语句"
          :autosize="{ minRows: 2, maxRows: 20 }"
          v-loading="uploading"
          @blur="saveSql"
        />
        <div style="margin-top:16px">
          <el-upload
            style="display:inline-block;margin-right:calc(100% - 200px)"
            action="#"
            accept=".sql"
            :disabled="uploading"
            :show-file-list="false"
            :http-request="uploadFile"
          >
            <el-button size="medium" type="file" style="width:100px"
              >读取文件</el-button
            >
          </el-upload>
          <el-button size="medium" type="primary" style="width:100px">
            运行 <em class="el-icon-d-arrow-right" />
          </el-button>
        </div>

        <el-row v-if="result_type > 0" style="margin-top:24px">
          <el-divider content-position="left">查询结果</el-divider>

          <!-- 返回表格 -->
          <div v-if="result_type === 1">
            <el-table fit stripe size="medium" :data="results">
              <el-table-column label="#" width="40">
                <span slot-scope="props">{{ props.$index + 1 }}</span>
              </el-table-column>
              <el-table-column
                v-for="(h, i) in headers"
                :key="i"
                :label="h"
                :prop="h"
              />
            </el-table>
            <p class="info">
              共 {{ results.length }} 条记录，查询用时 {{ queryTime }} 秒
            </p>
          </div>

          <!-- 返回消息 -->
          <div v-else-if="result_type === 2"></div>
        </el-row>
      </el-card>
    </el-col>

    <el-col :xs="24" :sm="24" :md="6">
      <el-card shadow="hover">
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
      // 查询语句
      statements: '',

      // 查询用时
      queryTime: 0.01,
      // 查询结果的标题
      headers: [] as string[],
      // 结果类型
      result_type: 0,
      // 查询结果
      results: [] as any[],

      // 正在上传文件
      uploading: false,
      // 正在查询
      loading: false,
    };
  },
  methods: {
    /** 上传文件 */
    uploadFile(request: any) {
      const file = request.file as File;
      if (file.size > 65536) {
        // 为了避免前端直接卡死
        this.$message({ message: '上传文件太大！', type: 'warning' });
        return;
      }
      this.uploading = true;
      const reader = new FileReader();
      reader.onload = (event) => {
        this.statements = (event.target as FileReader).result!.toString();
        this.saveSql();
        this.uploading = false;
      };
      reader.onerror = () => {
        this.$message({ message: '读取文件失败', type: 'error' });
        this.uploading = false;
      };
      reader.readAsText(file);
    },
    /** 保存语句到临时存储 */
    saveSql() {
      this.$store.commit('save', this.statements);
    },
  },
  mounted() {
    this.statements = this.$store.state.sql;
  },
});
</script>

<style lang="scss" scoped>
.info {
  margin-top: 8px;
  text-align: right;
  font-size: 10pt;
  color: #909399;
}
</style>
