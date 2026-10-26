const { app, BrowserWindow, shell, Menu } = require('electron');
const path = require('path');

function crear() {
  Menu.setApplicationMenu(null);
  const w = new BrowserWindow({
    width: 1000, height: 820, title: 'NX masters · Creador',
    webPreferences: { contextIsolation: true, nodeIntegration: false }
  });
  w.loadFile(path.join(__dirname, 'page', 'index.html'));
  // Los enlaces externos (crear token, etc.) se abren en el navegador.
  w.webContents.setWindowOpenHandler(({ url }) => { shell.openExternal(url); return { action: 'deny' }; });
  w.webContents.on('will-navigate', (e, url) => { if (!url.startsWith('file://')) { e.preventDefault(); shell.openExternal(url); } });
}
app.whenReady().then(crear);
app.on('window-all-closed', () => app.quit());
