const { app, BrowserWindow, shell, Menu, dialog, ipcMain } = require('electron');
const path = require('path');
const fs = require('fs');
const archiver = require('archiver');
let zipSt = null;

// Destino seguro: nunca sale de la carpeta elegida.
function destino(root, rel) {
  const base = path.resolve(root);
  const full = path.resolve(base, rel);
  if (full !== base && !full.startsWith(base + path.sep)) throw new Error('Ruta no permitida: ' + rel);
  return full;
}

ipcMain.handle('nx:elegir', async () => {
  const r = await dialog.showOpenDialog({ title: 'Elige tu SD o la carpeta destino', properties: ['openDirectory', 'createDirectory'] });
  return r.canceled ? null : r.filePaths[0];
});
ipcMain.handle('nx:escribir', async (_e, root, rel, data) => {
  const full = destino(root, rel);
  await fs.promises.mkdir(path.dirname(full), { recursive: true });
  await fs.promises.writeFile(full, Buffer.from(data));
  return true;
});
ipcMain.handle('nx:copiar', async (_e, src, root, rel) => {
  if (!src) throw new Error('No pude leer la ruta del archivo');
  const full = destino(root, rel);
  await fs.promises.mkdir(path.dirname(full), { recursive: true });
  await fs.promises.copyFile(src, full);
  return true;
});

ipcMain.handle('nx:leer', async (_e, src) => { const b = await fs.promises.readFile(src); return new Uint8Array(b); });
ipcMain.handle('nx:elegirZip', async (_e, nombre) => {
  const r = await dialog.showSaveDialog({ title: 'Guardar el zip completo', defaultPath: nombre, filters: [{ name: 'Zip', extensions: ['zip'] }] });
  return r.canceled ? null : r.filePath;
});
ipcMain.handle('nx:zipIniciar', async (_e, dest) => {
  const out = fs.createWriteStream(dest);
  const ar = archiver('zip', { zlib: { level: 0 }, forceZip64: true });
  const fin = new Promise((res, rej) => { out.on('close', res); out.on('error', rej); ar.on('error', rej); });
  ar.pipe(out);
  zipSt = { ar, fin };
  return true;
});
ipcMain.handle('nx:zipBuffer', async (_e, rel, data) => { zipSt.ar.append(Buffer.from(data), { name: rel.replace(/\\/g, '/') }); return true; });
ipcMain.handle('nx:zipArchivo', async (_e, src, rel) => {
  // espera a que termine de leerse cada archivo grande antes de seguir
  await new Promise((res, rej) => {
    const rs = fs.createReadStream(src);
    rs.on('error', rej);
    zipSt.ar.append(rs, { name: rel.replace(/\\/g, '/') });
    zipSt.ar.once('entry', () => res());
  });
  return true;
});
ipcMain.handle('nx:zipFin', async () => { await zipSt.ar.finalize(); await zipSt.fin; zipSt = null; return true; });

function crear() {
  Menu.setApplicationMenu(null);
  const w = new BrowserWindow({
    width: 1000, height: 820, title: 'NX masters · Creador',
    webPreferences: { contextIsolation: true, nodeIntegration: false, preload: path.join(__dirname, 'preload.js') }
  });
  w.loadFile(path.join(__dirname, 'page', 'index.html'));
  w.webContents.setWindowOpenHandler(({ url }) => { shell.openExternal(url); return { action: 'deny' }; });
  w.webContents.on('will-navigate', (e, url) => { if (!url.startsWith('file://')) { e.preventDefault(); shell.openExternal(url); } });
}
app.whenReady().then(crear);
app.on('window-all-closed', () => app.quit());
