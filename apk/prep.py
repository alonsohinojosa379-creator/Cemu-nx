# Copia la pagina a www/ y hace que "descargar zip" funcione dentro de la app
# (los enlaces blob: no se descargan en un WebView): guarda en cache y abre Compartir.
import os, shutil
os.makedirs('www', exist_ok=True)
s = open('page/index.html', encoding='utf-8').read()
shim = r'''<script>
(function(){
  var C = window.Capacitor; if(!C || !C.Plugins) return;
  var orig = HTMLAnchorElement.prototype.click;
  HTMLAnchorElement.prototype.click = function(){
    var a = this;
    if(a.download && a.href && a.href.indexOf('blob:') === 0){
      fetch(a.href).then(function(r){return r.blob();}).then(function(b){
        var fr = new FileReader();
        fr.onload = function(){
          var data = String(fr.result).split(',')[1];
          C.Plugins.Filesystem.writeFile({path: a.download, data: data, directory: 'CACHE'})
            .then(function(res){ return C.Plugins.Share.share({title: a.download, url: res.uri, dialogTitle: 'Guardar ' + a.download}); })
            .catch(function(e){ alert('No se pudo guardar: ' + (e && e.message || e)); });
        };
        fr.readAsDataURL(b);
      });
      return;
    }
    return orig.apply(this, arguments);
  };
})();
</script>
'''
assert '<script src="https://cdnjs' in s
s = s.replace('<script src="https://cdnjs', shim + '<script src="https://cdnjs', 1)
open('www/index.html', 'w', encoding='utf-8').write(s)
