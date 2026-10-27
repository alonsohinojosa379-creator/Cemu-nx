const { contextBridge, ipcRenderer, webUtils } = require('electron');
contextBridge.exposeInMainWorld('nx', {
  elegirCarpeta: () => ipcRenderer.invoke('nx:elegir'),
  escribir: (root, rel, data) => ipcRenderer.invoke('nx:escribir', root, rel, data),
  copiar: (src, root, rel) => ipcRenderer.invoke('nx:copiar', src, root, rel),
  rutaDe: (file) => webUtils.getPathForFile(file)
});
