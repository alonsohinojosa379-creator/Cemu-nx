// Prueba en el build: el zip grande (archiver) debe funcionar empaquetado, sin node_modules.
const { execSync } = require('child_process');
const fs = require('fs'), path = require('path'), os = require('os');
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'nxsmoke-'));
const t = `const archiver=require('archiver'),fs=require('fs');
const o=fs.createWriteStream(process.argv[2]);const ar=archiver('zip',{zlib:{level:0},forceZip64:true});
ar.pipe(o);ar.append(Buffer.from('hola'),{name:'a/b.txt'});ar.append(fs.createReadStream(process.argv[1]),{name:'c.txt'});ar.finalize();
o.on('close',()=>console.log('ZIPOK '+fs.statSync(process.argv[2]).size));`;
fs.writeFileSync(path.join(__dirname, 'smoke-src.js'), t);
execSync('npx esbuild smoke-src.js --bundle --platform=node --outfile=' + JSON.stringify(path.join(tmp, 'smoke.bundle.js')), { stdio: 'inherit' });
const out = execSync('node smoke.bundle.js ' + JSON.stringify(__filename) + ' ' + JSON.stringify(path.join(tmp, 'o.zip')), { cwd: tmp }).toString();
if (!out.includes('ZIPOK')) { console.error('smoke fallo: ' + out); process.exit(1); }
console.log(out);
fs.rmSync(path.join(__dirname, 'smoke-src.js'));
