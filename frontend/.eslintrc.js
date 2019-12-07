module.exports = {
  root: true,
  env: {
    node: true,
  },
  extends: ['plugin:vue/essential', '@vue/prettier', '@vue/typescript'],
  rules: {
    'no-console': process.env.NODE_ENV === 'production' ? 'error' : 'off',
    'no-debugger': process.env.NODE_ENV === 'production' ? 'error' : 'off',
    'prettier/prettier': [
      'warn',
      {
        printWidth: 100,
        singleQuote: true,
        quoteProps: 'consistent',
        trailingComma: 'all',
        arrowParens: 'always',
      },
    ],
  },
  parserOptions: {
    parser: '@typescript-eslint/parser',
  },
};
