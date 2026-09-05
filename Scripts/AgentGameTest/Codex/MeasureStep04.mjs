// Read-only local telemetry extraction. Does not export conversation contents.
import fs from 'node:fs';
import readline from 'node:readline';
const source='C:/Users/User/.codex/sessions/2026/08/27/rollout-2026-08-27T21-03-48-01a0431a-dc4b-7151-bbf8-45703cbcdbbb.jsonl';
const start='2026-09-02T12:40:11.000Z';
let before=null, first=null, last=null, count=0;
for await (const line of readline.createInterface({input:fs.createReadStream(source),crlfDelay:Infinity})) {
  if (!line.includes('"token_count"')) continue;
  let row; try {row=JSON.parse(line);} catch {continue;}
  if(row.type!=='event_msg'||row.payload.type!=='token_count'||!row.payload.info)continue;
  const point={timestamp:row.timestamp,info:row.payload.info};
  if(row.timestamp<start)before=point;
  else {first??=point;last=point;count++;}
}
if (!before||!last) throw new Error('No measured telemetry boundary');
const delta={};for(const k of Object.keys(last.info.total_token_usage))delta[k]=last.info.total_token_usage[k]-(before.info.total_token_usage[k]??0);
console.log(JSON.stringify({scope:'Root task reported cumulative usage delta; child usage not added separately',start_requested:start,boundary_before:before.timestamp,first_event:first.timestamp,last_event:last.timestamp,event_count:count,delta,context_before:{last_input_tokens:before.info.last_token_usage.input_tokens,model_context_window:before.info.model_context_window},context_last:{last_input_tokens:last.info.last_token_usage.input_tokens,model_context_window:last.info.model_context_window}},null,2));
