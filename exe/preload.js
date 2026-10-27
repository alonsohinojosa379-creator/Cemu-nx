const { contextBridge, ipcRenderer, webUtils } = require('electron');
contextBridge.exposeInMainWorld('nx', {
  elegirCarpeta: () => ipcRenderer.invoke('nx:elegir'),
  escribir: (root, rel, data) => ipcRenderer.invoke('nx:escribir', root, rel, data),
  copiar: (src, root, rel) => ipcRenderer.invoke('nx:copiar', src, root, rel),
  elegirZip: (nombre) => ipcRenderer.invoke('nx:elegirZip', nombre),
  zipIniciar: (d) => ipcRenderer.invoke('nx:zipIniciar', d),
  zipBuffer: (r, d) => ipcRenderer.invoke('nx:zipBuffer', r, d),
  zipArchivo: (s, r) => ipcRenderer.invoke('nx:zipArchivo', s, r),
  zipFin: () => ipcRenderer.invoke('nx:zipFin'),
  rutaDe: (file) => webUtils.getPathForFile(file)
});
