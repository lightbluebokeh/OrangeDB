<template>
  <div id="app">
    <el-container>
      <el-header>
        <el-menu :default-active="$route.path" router mode="horizontal">
          <!-- 来点emoji -->
          <el-menu-item index="/">🍊 OrangeDB</el-menu-item>
          <el-menu-item index="/about">关于</el-menu-item>
        </el-menu>
      </el-header>

      <el-main class="body">
        <el-scrollbar class="scrollbar">
          <router-view />
        </el-scrollbar>
      </el-main>
    </el-container>
  </div>
</template>

<script lang="ts">
import Vue from 'vue';
export default Vue.extend({});
</script>

<style lang="scss">
$header-height: 60px;

.body {
  padding: 0;
  // 鬼知道body的8px margin是从哪来的
  height: calc(100vh - #{$header-height} - 16px);
}

.scrollbar {
  height: 100%;

  .el-scrollbar__wrap {
    overflow-x: hidden;
  }
}
</style>
