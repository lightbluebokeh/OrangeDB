<template>
  <div id="app">
    <el-container>
      <el-header>
        <el-menu :default-active="$route.path" router mode="horizontal">
          <el-menu-item index="/">🍊 OrangeDB</el-menu-item>
          <el-menu-item index="/about">关于</el-menu-item>
        </el-menu>
      </el-header>

      <el-main class="main">
        <el-scrollbar class="scrollbar">
          <router-view />
        </el-scrollbar>
      </el-main>
    </el-container>
  </div>
</template>

<style lang="scss">
$header-height: 60px;

.main {
  padding: 0;
  // body的8px margin
  height: calc(100vh - #{$header-height} - 16px);
}

.scrollbar {
  height: 100%;

  .el-scrollbar__wrap {
    overflow-x: hidden;
  }
}
</style>
