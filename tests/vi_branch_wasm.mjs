import fs from "node:fs";
import path from "node:path";

const [modules, recordings, fixture, expected, count, ...addresses] = process.argv.slice(2);
const program = fs.readFileSync(fixture);
const expectedRows = new Map(fs.readFileSync(expected, "utf8").trim().split(/\r?\n/).map(line => line.split(" ")));
const mem = new WebAssembly.Memory({initial: 2, maximum: 65536, shared: true});
const regs = 0x2000, data = 0x4000;
const words = new DataView(mem.buffer);
words.setUint32(regs + 12, 0x3f800000, true);
words.setUint32(data + 0x20 * 16 + 8, 1, true);
let top = 0x40;
const env = {
  memory: mem, top: () => top, itop: () => 3,
  mac: () => 0, status: () => 0, clip: () => 0,
  force_mac: () => {}, efu: () => { throw new Error("unexpected EFU"); },
  kick: () => { throw new Error("unexpected XGKICK"); },
  interpret: (_, pc) => { throw new Error(`unexpected fallback at ${pc.toString(16)}`); },
};
const programs = new Map(fs.readdirSync(modules).filter(n=>n.endsWith(".wasm") && fs.readFileSync(path.join(recordings,n.replace(/\.wasm$/,".bin"))).subarray(0,program.length - 8).equals(program.subarray(0,program.length - 8))).map(n=>[parseInt(n.match(/_([0-9a-f]+)\.wasm/i)[1],16),n]));
for (let pos = 24; pos <= Number(count) * 24; pos += 24) {
  const entry = pos * 8;
  const filename = programs.get(entry);
  if (!filename) throw new Error(`missing VU program ${entry.toString(16)}`);
  const m = new WebAssembly.Module(fs.readFileSync(path.join(modules,filename)));
  const instance = new WebAssembly.Instance(m,{env});
  instance.exports.run(regs,data);
  top = top === 0x40 ? 0x50 : 0x40;
}
for (const name of addresses) {
  const offset = data + parseInt(name,16) * 16;
  const actual = Buffer.from(mem.buffer,offset,16).toString("hex");
  console.log(`${name} ${actual}`);
  if (actual !== expectedRows.get(name)) throw new Error(`${name} differs from PCSX2`);
}
