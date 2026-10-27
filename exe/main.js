const { app, BrowserWindow, shell, Menu, dialog, ipcMain } = require('electron');
const path = require('path');
const fs = require('fs');

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
