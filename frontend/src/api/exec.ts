import axios from 'axios';

export interface ExecResult {
  // 错误信息
  error?: string;
  // 标题
  headers?: string[];
  // 结果
  data?: any[][];
}

export interface ExecResponse {
  // 执行时间
  time: number;
  // 结果
  results: ExecResult[];
}

export default async function exec(sql: string): Promise<ExecResponse> {
  const { data } = await axios.post('/api/exec', { sql });
  if (data && typeof data.time === 'number' && data.results) {
    data.results = JSON.parse(data.results);
    return data;
  }
  throw new Error('what?');
}
