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
          :autosize="{ minRows: results.length ? 3 : 8, maxRows: 20 }"
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
            <el-button size="medium" type="file" style="width:100px">读取文件</el-button>
          </el-upload>
          <el-button size="medium" type="primary" style="width:100px" @click="execSql">
            运行 <em class="el-icon-d-arrow-right" />
          </el-button>
        </div>

        <el-row v-if="results.length > 0" v-loading="loading" style="margin-top:24px">
          <el-divider content-position="left">查询结果</el-divider>

          <el-tabs type="border-card">
            <el-tab-pane v-for="(r, ri) in results" :key="ri">
              <em v-if="r.error" slot="label" class="el-icon-circle-close danger" />
              <span v-else slot="label">{{ ri + 1 }}</span>

              <el-table v-if="r.data" fit stripe size="medium" :data="r.data">
                <el-table-column label="#" width="40">
                  <span slot-scope="props" class="info">{{ props.$index + 1 }}</span>
                </el-table-column>
                <el-table-column v-for="(h, i) in r.headers" :key="i" :label="h">
                  <span slot-scope="props">{{ props.row[i] }}</span>
                </el-table-column>
              </el-table>
              <div v-else style="font-size:11pt">
                <p class="danger"><em class="el-icon-warning-outline" /> 运行错误：</p>
                <p style="margin-left:20px">{{ r.error }}</p>
              </div>

              <p v-if="r.data" class="info text">共 {{ r.data.length }} 条记录</p>
            </el-tab-pane>
          </el-tabs>
          <p class="info text">查询用时 {{ queryTime }} 秒</p>
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
import exec, { ExecResult } from '@/api/exec';
export default Vue.extend({
  data() {
    return {
      // 查询语句
      statements: '',

      // 查询用时
      queryTime: 0,
      // 查询结果
      results: [] as ExecResult[],

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
    /** 执行sql */
    execSql: async function() {
      try {
        this.loading = true;
        const r = await exec(this.statements);
        this.queryTime = r.time;
        this.results = r.results;
      } catch (error) {
        console.error(error);
        this.$message({ message: '出了点小问题？', type: 'error' });
      } finally {
        this.loading = false;
      }
    },
  },
  mounted() {
    this.statements = this.$store.state.sql; // 加载保存的sql
  },
});
</script>

<style lang="scss" scoped>
.text {
  margin-top: 12px;
  text-align: right;
  font-size: 10pt;
  color: #909399;
}
</style>
