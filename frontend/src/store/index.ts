import Vue from 'vue';
import Vuex from 'vuex';

Vue.use(Vuex);

export default new Vuex.Store({
  state: {
    sql: '',
  },
  mutations: {
    save(state, sql) {
      state.sql = sql;
    },
  },
  actions: {},
  modules: {},
});
